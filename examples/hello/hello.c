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

#include "../../include/cebsocket.h"

void on_data(cebsocket_clients_t* client, char* data) {
    printf("WebSocket Message: %s\n", data);
    
    cebsocket_send(client, "Hello from WebSocket server!");
}

void on_connected(cebsocket_clients_t* client) {
    printf("Client connected #%d\n", client->id);
}

void on_disconnected(cebsocket_clients_t* client) {
    printf("Client disconnected #%d\n", client->id);
}

int main() {
    printf("Starting WebSocket server..\n");

    cebsocket_t* ws = cebsocket_init(8080);

    ws->on_data = on_data;
    ws->on_connected = on_connected;
    ws->on_disconnected = on_disconnected;
    
    cebsocket_listen(ws);
    
    return 0;
}