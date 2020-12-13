/*
 * cebsocket is a lightweight WebSocket library for C
 *
 * https://github.com/rohanrhu/cebsocket
 *
 * Licensed under MIT
 * Copyright (C) 2020, Oğuzhan Eroğlu (https://oguzhaneroglu.com/) <rohanrhu2@gmail.com>
 *
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <openssl/sha.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#include "../include/cebsocket.h"
#include "../include/util.h"

unsigned long long clients_id_i = 0;

extern cebsocket_t* cebsocket_init(int port) {
    cebsocket_t* ws = malloc(sizeof(cebsocket_t));
    ws->port = port;
    ws->host_address = NULL;
    ws->bind_address = NULL;
    ws->on_data = NULL;
    ws->on_connected = NULL;
    ws->on_disconnected = NULL;

    return ws;
}

extern void cebsocket_listen(cebsocket_t* ws) {
    sigaction(SIGPIPE, &(struct sigaction){broken_pipe_handler}, NULL);
    
    int server_socket;
    int client_socket;
    int client_addr_len;

    struct sockaddr_in serv_addr;
    struct sockaddr_in cli_addr;

    server_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (server_socket < 0) {
        perror("Socket error");
        exit(1);
    }

    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));

    memset(&serv_addr, 0, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(ws->port);

    if (ws->bind_address) {
        serv_addr.sin_addr.s_addr = inet_addr(ws->bind_address);
    } else {
        serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    }

    if (bind(server_socket, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Bind error.");
        exit(1);
    }

    listen(server_socket, 10);
    client_addr_len = sizeof(cli_addr);

    cebsocket_clients_t* clients = NULL;

    ws->clients = clients;

    cebsocket_util_verbose("WebSocket server is listening from 0.0.0.0:%d\n", ws->port);

    pthread_t client_thread;

    char str_addr[INET6_ADDRSTRLEN+1];

    cebsocket_clients_t* client = NULL;
    cebsocket_clients_t* current_client = NULL;

    for (;;) {
        client_socket = accept(server_socket, (struct sockaddr *) &cli_addr, &client_addr_len);
        if (client_socket < 0) {
            perror("Accept error");
            exit(1);
        }

        inet_ntop(AF_INET, (void*)&cli_addr.sin_addr, str_addr, INET_ADDRSTRLEN);

        client = cebsocket_client_init();
        client->id = ++clients_id_i;
        client->ws = ws;
        client->server_socket = server_socket;
        client->is_host = 0;
        client->socket = client_socket;
        client->address = cli_addr.sin_addr.s_addr;

        cebsocket_util_verbose("Client connected: #%d (%s)\n", client->id, str_addr);

        client->ws->on_connected(client);

        if (!clients) {
            clients = client;
        } else {
            client->prev = current_client;
            current_client->next = client;
        }

        current_client = client;

        pthread_create(
            &client_thread,
            NULL,
            (void *) &client_handler,
            (void *) client
        );

        pthread_detach(client_thread);
    }
}

extern void cebsocket_send(cebsocket_clients_t* client, char* message) {
    cebsocket_util_verbose("Sending to #%d (%d): %s\n", client->id, client->address, message);
    
    unsigned int message_len = strlen(message);

    if (message_len < 126) {
        cebsocket_packet_frame_len8_t message_frame;
        message_frame.fin_rsv_opcode = 0b10000001;
        message_frame.mlen8 = message_len;

        send(client->socket, &message_frame, sizeof(cebsocket_packet_frame_len8_t), 0);
    } else if (message_len < (1 << 16)) {
        cebsocket_packet_frame_len16_t message_frame;
        message_frame.fin_rsv_opcode = 0b10000001;
        message_frame.mlen8 = 126;
        message_frame.len16 = message_len;

        send(client->socket, &message_frame, sizeof(cebsocket_packet_frame_len16_t), 0);
    } else {
        cebsocket_packet_frame_len64_t message_frame;
        message_frame.fin_rsv_opcode = 0b10000001;
        message_frame.mlen8 = 127;
        message_frame.len64 = message_len;

        send(client->socket, &message_frame, sizeof(cebsocket_packet_frame_len64_t), 0);
    }
    
    send(client->socket, message, strlen(message), 0);
}

static void client_handler(cebsocket_clients_t* client) {
    sigaction(SIGPIPE, &(struct sigaction){broken_pipe_handler}, NULL);
    receive_http_packet(client);
}

static void receive_http_packet(cebsocket_clients_t* client) {
    char str_addr[INET6_ADDRSTRLEN+1];
    inet_ntop(AF_INET, (void*)&client->address, str_addr, INET_ADDRSTRLEN);
    
    ssize_t result;

    int is_cr = 0;
    int is_lf = 0;

    char header_buff[HTTP_HEADER_BUFF_SIZE+1];
    char prop_buff[HTTP_PROP_BUFF_SIZE+1];
    char val_buff[HTTP_PROP_BUFF_SIZE+1];

    int header_buff_i = 0;
    int prop_buff_i = 0;
    int val_buff_i = 0;

    enum CEBSOCKET_HTTP_HEADER_PARSER_STATE
    parser_state = CEBSOCKET_HTTP_HEADER_PARSER_STATE_METHOD;

    char byte;

    RECEIVE_PACKET:

    result = recv(client->socket, &byte, 1, MSG_WAITALL);

    if (!result) {
        cebsocket_util_verbose("Client disconnected: %s\n", str_addr);

        client->ws->on_disconnected(client);
        
        return;
    }

    if (parser_state == CEBSOCKET_HTTP_HEADER_PARSER_STATE_METHOD) {
        if (!is_cr) {
            if (byte == '\r') {
                is_cr = 1;
            }
        } else {
            if (byte == '\n') {
                parser_state = CEBSOCKET_HTTP_HEADER_PARSER_STATE_PROP;
                goto RECEIVE_PACKET;
            } else {
                close(client->socket);
                return;
            }
        }
        
        *(header_buff+(header_buff_i++)) = byte;

        if (!is_lf) {
            goto RECEIVE_PACKET;
        }

        is_lf = 0;

        const char* expected_header = "GET / HTTP/1.1";

        if (strncmp(header_buff, expected_header, sizeof("GET / HTTP/1.1")-1) == 0) {
            goto RECEIVE_PACKET;
        } else {
            close(client->socket);
            return;
        }

        goto RECEIVE_PACKET;
    } else if (parser_state == CEBSOCKET_HTTP_HEADER_PARSER_STATE_PROP) {
        if (byte == '\r') {
            parser_state = CEBSOCKET_HTTP_HEADER_PARSER_STATE_END;
        } if (byte != ':') {
            *(prop_buff+(prop_buff_i++)) = byte;
        } else {
            parser_state = CEBSOCKET_HTTP_HEADER_PARSER_STATE_SPACE;
            *(prop_buff+(prop_buff_i)) = '\0';
            prop_buff_i = 0;
        }
    } else if (parser_state == CEBSOCKET_HTTP_HEADER_PARSER_STATE_SPACE) {
        if (byte != ' ') {
            close(client->socket);
            return;
        } else {
            parser_state = CEBSOCKET_HTTP_HEADER_PARSER_STATE_VAL;
        }
    } else if (parser_state == CEBSOCKET_HTTP_HEADER_PARSER_STATE_VAL) {
        if (byte == '\r') {
            parser_state = CEBSOCKET_HTTP_HEADER_PARSER_STATE_CR;
            *(val_buff+(val_buff_i)) = '\0';
            val_buff_i = 0;
        } else {
            *(val_buff+(val_buff_i++)) = byte;
        }
    } else if (parser_state == CEBSOCKET_HTTP_HEADER_PARSER_STATE_CR) {
        if (byte == '\n') {
            parser_state = CEBSOCKET_HTTP_HEADER_PARSER_STATE_PROP;

            if (strcmp(prop_buff, "Sec-WebSocket-Key") == 0) {
                int len = strlen(val_buff);
                client->ws_key = malloc(len+1);
                strcpy(client->ws_key, val_buff);
            }
        } else {
            close(client->socket);
            return;
        }
    } else if (parser_state == CEBSOCKET_HTTP_HEADER_PARSER_STATE_END) {
        if (byte == '\n') {
            char response_buf[500];

            char concated[100];
            unsigned char hash[SHA_DIGEST_LENGTH+1];
            char* accept;

            sprintf(concated, "%s%s", client->ws_key, cebsocket_GUID);

            hash[SHA_DIGEST_LENGTH] = '\0';

            SHA1(concated, strlen(concated), hash);
            
            accept = cebsocket_util_base64_encode(hash);
            
            sprintf(response_buf, "HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: %s\r\nCompression: None\r\n\r\n", accept);

            send(client->socket, response_buf, strlen(response_buf), 0);
            
            goto RECEIVE_PACKET_WS;
        }
    }

    goto RECEIVE_PACKET;

    RECEIVE_PACKET_WS:

    receive_ws_packet(client);
}

static void receive_ws_packet(cebsocket_clients_t* client) {
    char str_addr[INET6_ADDRSTRLEN+1];
    inet_ntop(AF_INET, (void*)&client->address, str_addr, INET_ADDRSTRLEN);
    
    ssize_t result;

    uint16_t header0_16;
    uint64_t plen;
    uint16_t plen16;
    uint16_t plen64;
    uint8_t opcode;
    uint8_t is_masked;
    unsigned char mkey[4];

    char* req = NULL;
    char* res = NULL;

    RECEIVE_FRAME:

    result = recv(client->socket, &header0_16, 2, MSG_WAITALL);
    if (!result) {
        client_disconnected(client);
        return;
    }

    opcode = ((uint8_t)header0_16) & 0b00001111;

    is_masked = (*(((uint8_t*)(&header0_16))+1)) & -128;
    plen = (*(((uint8_t*)(&header0_16))+1)) & 127;
    
    if (plen == 126) {
        result = recv(client->socket, &plen16, 2, MSG_WAITALL);
        if (!result) {
            client_disconnected(client);
            return;
        }

        plen = ntohs(plen16);
    } else if (plen == 127) {
        result = recv(client->socket, &plen64, 8, MSG_WAITALL);
        if (!result) {
            client_disconnected(client);
            return;
        }

        plen = ntohs(plen64);
    }

    if (is_masked) {
        result = recv(client->socket, mkey, 4, MSG_WAITALL);
        if (!result) {
            client_disconnected(client);
            return;
        }
    }

    req = malloc(plen+1);
    req[plen] = '\0';

    result = recv(client->socket, req, plen, MSG_WAITALL);
    if (!result) {
        client_disconnected(client);
        return;
    }

    if (is_masked) {
        for (int i=0; i < plen; i++) {
            req[i] = req[i] ^ mkey[i%4];
        }
    }

    cebsocket_util_verbose("Data from #%d: %s\n", client->id, req);

    if (client->ws->on_data)
        client->ws->on_data(client, req);

    goto RECEIVE_FRAME;
}

static void client_disconnected(cebsocket_clients_t* client) {
    char str_addr[INET6_ADDRSTRLEN+1];
    inet_ntop(AF_INET, (void*)&client->address, str_addr, INET_ADDRSTRLEN);
    
    cebsocket_util_verbose("Client disconnected: #%d (%s)\n", client->id, str_addr);
    client->ws->on_disconnected(client);
    cebsocket_client_free(client);
}

cebsocket_clients_t* cebsocket_client_init() {
    cebsocket_clients_t* client = malloc(sizeof(cebsocket_clients_t));
    client->socket = 0;
    client->ws_key = NULL;
    client->address = 0;
    client->next = NULL;
    client->prev = NULL;
}

void cebsocket_client_free(cebsocket_clients_t* client) {
    if (client->next) {
        client->next->prev = client->prev;
    }
    
    if (client->prev) {
        client->prev->next = client->next;
    }

    if (client->ws_key != NULL) {
        free(client->ws_key);
    }

    free(client);
}

void cebsocket_free(cebsocket_t* ws) {
    free(ws);
}

void broken_pipe_handler(int sig) {
    cebsocket_util_verbose("Broken pipe.\n");
}