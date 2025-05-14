#include "net/icmp.h"
#include "io/ports.h"

void icmp_handle_packet(SAYORI_UNUSED netcard_entry_t* card, SAYORI_UNUSED char* packet_data) {
	qemu_log("Type: %x", *packet_data);
	qemu_log("Code: %x", *(packet_data + 1));
}