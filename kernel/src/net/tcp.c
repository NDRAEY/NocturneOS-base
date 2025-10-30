//
// Created by ndraey on 21.4.2024.
//

#include "common.h"
#include "net/cards.h"
#include "net/ethernet.h"
#include "net/tcp.h"
#include "net/endianess.h"
#include <io/logging.h>
#include "net/ipv4.h"
#include "mem/vmm.h"
#include "lib/rand.h"


#define MAX_CONNECTIONS 64

volatile tcp_connection_t TCP_CONNECTIONS[MAX_CONNECTIONS] = {};

int tcp_find_connection(uint8_t address[4], size_t port) {
	for(int i = 0; i < MAX_CONNECTIONS; i++) {
		if(memcmp((const char*)&TCP_CONNECTIONS[i].dest_ip_addr, (char*)address, 4) == 0
			&& (TCP_CONNECTIONS[i].source_port == port || TCP_CONNECTIONS[i].dest_port == port)
			&& TCP_CONNECTIONS[i].status != TCP_NONE) {
			return i;
		}
	}

	return -1;
}

bool tcp_new_connection(netcard_entry_t* card, uint8_t address[4], size_t port, size_t seq_nr) {
	uint8_t empty_addr[4] = {0, 0, 0, 0};
	int index = -1;

	for(int i = 0; i < MAX_CONNECTIONS; i++) {
		if(memcmp((const char*)&TCP_CONNECTIONS[i].dest_ip_addr, (const char*)empty_addr, 4) == 0
				&& TCP_CONNECTIONS[i].source_port == 0
				&& TCP_CONNECTIONS[i].status == TCP_NONE) {
			index = i;
			break;
		}
	}

	if(index == -1) {
		return false;
	}

	memcpy((void*)&TCP_CONNECTIONS[index].dest_ip_addr, address, 4);
	TCP_CONNECTIONS[index].source_port = port;
	TCP_CONNECTIONS[index].status = TCP_CREATED;
	TCP_CONNECTIONS[index].seq = seq_nr;
	TCP_CONNECTIONS[index].card = card;

	return true;
}

typedef struct {
    uint32_t src_addr;
    uint32_t dst_addr;
    uint8_t reserved;
    uint8_t protocol;
    uint16_t tcp_length;
} __attribute__((packed)) ipv4_pseudo_header_t;

uint16_t checksum(void *data, size_t len) {
    uint32_t sum = 0;
    uint16_t *ptr = (uint16_t *)data;

    while (len > 1) {
        sum += *ptr++;
        len -= 2;
    }

    // If odd byte left, pad with zero
    if (len > 0) {
        sum += *((uint8_t *)ptr);
    }

    // Fold 32-bit sum to 16-bit
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    return (uint16_t)(~sum);  // One's complement
}

// TODO: !!!
uint16_t tcp_calculate_checksum(uint32_t src_addr, uint32_t dst_addr, tcp_packet_t *tcp_packet, uint16_t tcp_length) {
    ipv4_pseudo_header_t pseudo_header;
    pseudo_header.src_addr = src_addr;
    pseudo_header.dst_addr = dst_addr;
    pseudo_header.reserved = 0;
    pseudo_header.protocol = 6; // TCP protocol number
    pseudo_header.tcp_length = htons(tcp_length);

    size_t buffer_size = sizeof(ipv4_pseudo_header_t) + tcp_length;
    uint8_t *buffer = kmalloc(buffer_size);
    if (!buffer) {
        return 0; // Handle allocation failure
    }

    memcpy(buffer, &pseudo_header, sizeof(ipv4_pseudo_header_t));
    memcpy(buffer + sizeof(ipv4_pseudo_header_t), tcp_packet, tcp_length);

    uint16_t check = checksum(buffer, buffer_size);
    kfree(buffer);
    return check;
}

void tcp_handle_packet(netcard_entry_t *card, tcp_packet_t *packet) {
	qemu_note("!!!!!!!!!!!!!!!!!!!!!!!! TCP !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");

	ipv4_packet_t *ipv4 = (ipv4_packet_t *)((size_t)packet - sizeof(ipv4_packet_t));
	size_t data_payload_size = ipv4->TotalLength - sizeof(ipv4_packet_t);

	qemu_log("Data payload size: %d", data_payload_size);

	packet->source = ntohs(packet->source);
	packet->destination = ntohs(packet->destination);

	qemu_log("FROM: %u.%u.%u.%u", ipv4->Source[0], ipv4->Source[1], ipv4->Source[2], ipv4->Source[3]);

	qemu_note("SRC: %d; DEST: %d", packet->source, packet->destination);

	qemu_note("FLAGS: SYN: %d; ACK: %d; PSH: %d; FIN: %d", packet->syn, packet->ack, packet->psh, packet->fin);

	packet->ack_seq = ntohl(packet->ack_seq);
	packet->seq = ntohl(packet->seq);

	int idx = -1;
	uint16_t sp_port = ntohs(packet->source);
	if(tcp_find_connection(ipv4->Source, sp_port) == -1) {
		tcp_new_connection(card, ipv4->Source, sp_port, packet->seq);
		qemu_ok("Created new connection (port: %d)!", sp_port);
	}
	idx = tcp_find_connection(ipv4->Source, sp_port);

	tcp_connection_status_t current_status = TCP_CONNECTIONS[idx].status;
	
	qemu_note("Connection idx: %d; Status: %x", idx, current_status);
	
	// Stage 1. Packet arrived to create connection with us.
	//          SYN is set; Connection is created by `tcp_new_connection` automatically.
	//          We send SYN + ACK and wait for ACK. Mark connection as tcp_connection_status_t::TCP_CREATED.
	bool is_stage_1 = current_status == TCP_CREATED && packet->syn && !packet->ack && !packet->psh && !packet->fin;
	
	// Stage 2. Packet arrived with connection establishment confirmation.
	//          ACK is set; Connection is known as tcp_connection_status_t::TCP_CREATED;
	//          We send nothing. Mark connection as tcp_connection_status_t::TCP_ESTABLISHED.
	bool is_stage_2 = current_status == TCP_CREATED && !packet->syn && packet->ack && !packet->psh && !packet->fin;
	// bool is_push = !packet->syn && !packet->ack && packet->psh && !packet->fin;

	if(is_stage_1) {
		qemu_note("== STAGE 1 ==");

		tcp_packet_t* sendable_packet = kcalloc(sizeof(tcp_packet_t)/* + 8*/, 1);
		memcpy(sendable_packet, packet, sizeof(tcp_packet_t));	

		TCP_CONNECTIONS[idx].seq = rand();
		TCP_CONNECTIONS[idx].ack = packet->seq + 1; // Use the received packet's seq (host order)
	
		sendable_packet->ack = 1;
		sendable_packet->syn = 1; // Ensure SYN is set in response
		sendable_packet->seq = htonl(TCP_CONNECTIONS[idx].seq);
		sendable_packet->ack_seq = htonl(TCP_CONNECTIONS[idx].ack);
	
		uint16_t dest_port = sendable_packet->destination;
		uint16_t src_port = sendable_packet->source;
		sendable_packet->source = ntohs(dest_port);
		sendable_packet->destination = ntohs(src_port);
	
		sendable_packet->doff = 5; // Header length (5 * 4 = 20 bytes)
	
		// Calculate checksum with correct IPs and TCP length
		uint32_t src_ip, dst_ip;
		memcpy(&src_ip, ipv4->Destination, 4); // Server's IP (from received packet's destination)
		memcpy(&dst_ip, ipv4->Source, 4);      // Client's IP (from received packet's source)
		uint16_t tcp_length = sizeof(tcp_packet_t); // Adjust if options are added
	
		sendable_packet->check = 0; // Reset checksum before calculation
		sendable_packet->check = tcp_calculate_checksum(src_ip, dst_ip, sendable_packet, tcp_length);
	
		ipv4_send_packet(TCP_CONNECTIONS[idx].card, ipv4->Source, sendable_packet, sizeof(tcp_packet_t), ETH_IPv4_HEAD_TCP);

		kfree(sendable_packet);

		// TCP_CONNECTIONS[idx].source_port = ntohs(packet->destination);
		TCP_CONNECTIONS[idx].dest_port = ntohs(packet->destination);
		TCP_CONNECTIONS[idx].status = TCP_CREATED;
	} else if(is_stage_2) {
		qemu_note("== STAGE 2 ==");

		volatile tcp_connection_t* connection = TCP_CONNECTIONS + idx;

		connection->status = TCP_ESTABLISHED;

		qemu_ok("Connection established!");

		char* data = "Hello, world!\n";
		size_t length = strlen(data);

		tcp_send_packet((tcp_connection_t*)(TCP_CONNECTIONS + idx), data, length);
	} else {
		qemu_note("== ANOTHER STAGE! ==");
		qemu_note("== ANOTHER STAGE! ==");
		qemu_note("== ANOTHER STAGE! ==");
		qemu_note("== ANOTHER STAGE! ==");
		while(1);
	}

	qemu_log("Finished handling");
}

void tcp_send_packet(tcp_connection_t* connection, void* data, size_t len) {
	qemu_note("S: %d; D: %d", connection->source_port, connection->dest_port);

	tcp_packet_t* sendable_packet = kcalloc(sizeof(tcp_packet_t) + len, 1);
	// memcpy(sendable_packet, packet, sizeof(tcp_packet_t));

	char* inner_data = (char*)(sendable_packet + 1);

	memcpy(inner_data, data, len);

	// qemu_log("CS: %x; CA: %x; PS: %x; PA: %x", connection->seq, connection->ack, packet->seq, packet->ack_seq);
	// qemu_log("Was: %x; %x", connection->seq, connection->ack);

	connection->seq += 1;

	// qemu_log("Became: %x; %x (Should be: %x; %x)", connection->seq, connection->ack, packet->ack_seq, packet->seq);

	sendable_packet->source = connection->dest_port;
	sendable_packet->destination = connection->source_port;
	sendable_packet->window = 0xfffa;
	sendable_packet->psh = 1;
	sendable_packet->ack = 1;
	sendable_packet->syn = 0;
	sendable_packet->seq = htonl(connection->seq);
	sendable_packet->ack_seq = htonl(connection->ack);

	sendable_packet->doff = 5; // Header length (5 * 4 = 20 bytes)

	// Calculate checksum with correct IPs and TCP length
	uint32_t src_ip, dst_ip;
	memcpy(&src_ip, connection->card->ipv4_addr, 4); // Server's IP (from received packet's destination)
	memcpy(&dst_ip, &connection->dest_ip_addr, 4);      // Client's IP (from received packet's source)
	uint16_t tcp_length = sizeof(tcp_packet_t); // Adjust if options are added

	// CLARIFICATION: We set this field to zero to calculate checksum - the field should be zeroed.
	//                If not, the checksum will be always invalid.
	sendable_packet->check = 0;
	sendable_packet->check = tcp_calculate_checksum(src_ip, dst_ip, sendable_packet, tcp_length + len);

	ipv4_send_packet(connection->card, (uint8_t*)&connection->dest_ip_addr, sendable_packet, sizeof(tcp_packet_t) + len, ETH_IPv4_HEAD_TCP);

	kfree(sendable_packet);
}