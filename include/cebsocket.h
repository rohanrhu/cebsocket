/*
 * cebsocket is a lightweight WebSocket library for C
 *
 * https://github.com/rohanrhu/cebsocket
 *
 * Licensed under MIT
 * Copyright (C) 2020, Oğuzhan Eroğlu (https://oguzhaneroglu.com/) <rohanrhu2@gmail.com>
 *
 */

#ifndef __CEBSOCKET_H__
#define __CEBSOCKET_H__

#include <bits/stdint-uintn.h>

#define cebsocket_packet_frame_header \
    uint8_t fin_rsv_opcode; \
    uint8_t mlen8;
;

typedef struct cebsocket_packet_frame_len8 cebsocket_packet_frame_len8_t;
struct cebsocket_packet_frame_len8 {
    cebsocket_packet_frame_header
};

typedef struct cebsocket_packet_frame_len16 cebsocket_packet_frame_len16_t;
struct cebsocket_packet_frame_len16 {
    cebsocket_packet_frame_header
    uint16_t len16;
};

typedef struct cebsocket_packet_frame_len64 cebsocket_packet_frame_len64_t;
struct cebsocket_packet_frame_len64 {
    cebsocket_packet_frame_header
    uint64_t len64;
};

#define cebsocket_GUID "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"

#define HTTP_HEADER_BUFF_SIZE 700
#define HTTP_PROP_BUFF_SIZE 40
#define HTTP_VAL_BUFF_SIZE  50

enum CEBSOCKET_HTTP_HEADER_PARSER_STATE {
    CEBSOCKET_HTTP_HEADER_PARSER_STATE_METHOD = 1,
    CEBSOCKET_HTTP_HEADER_PARSER_STATE_PROP,
    CEBSOCKET_HTTP_HEADER_PARSER_STATE_SPACE,
    CEBSOCKET_HTTP_HEADER_PARSER_STATE_VAL,
    CEBSOCKET_HTTP_HEADER_PARSER_STATE_CR,
    CEBSOCKET_HTTP_HEADER_PARSER_STATE_END
};

enum CEBSOCKET_RESPONSE {
    CEBSOCKET_RESPONSE_INSTANCE_PORT = 1
};

typedef struct cebsocket cebsocket_t;
typedef struct cebsocket_clients cebsocket_clients_t;

struct cebsocket {
    int port;
    char* host_address;
    char* bind_address;
    cebsocket_clients_t* clients;
    void (*on_data)(cebsocket_clients_t*, char*);
    void (*on_connected)(cebsocket_clients_t*);
    void (*on_disconnected)(cebsocket_clients_t*);
};

struct cebsocket_clients {
    cebsocket_t* ws;
    int id;
    int socket;
    int server_socket;
    int address;
    char* ws_key;
    int is_host;
    cebsocket_clients_t* prev;
    cebsocket_clients_t* next;
};

extern cebsocket_t* cebsocket_init(int port);
extern void cebsocket_listen(cebsocket_t* ws);
extern void cebsocket_send(cebsocket_clients_t* client, char* message);

cebsocket_clients_t* cebsocket_client_init();
void cebsocket_free(cebsocket_t* ws);
void cebsocket_client_free(cebsocket_clients_t* client);

static void client_handler(cebsocket_clients_t* client);
static void receive_http_packet(cebsocket_clients_t* client);
static void receive_ws_packet(cebsocket_clients_t* client);
static void client_disconnected(cebsocket_clients_t* client);

void broken_pipe_handler(int sig);

#endif