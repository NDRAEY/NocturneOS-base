#pragma once

#define SOCKET_TCP 1

typedef struct {
    uint8_t address[4];
    uint16_t port;
} socket_address_t;

typedef struct {
    socket_address_t* bond_address;
    // ...
} socket_t;

socket_t* socket_new(socket_address_t* address, uint32_t flags);
socket_t* socket_listen(socket_t* server_socket);
void socket_read_until_newline(socket_t* socket, uint8_t* data);
void socket_close(socket_t* socket);