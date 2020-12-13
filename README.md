# `cebsocket`: a lightweight websocket library for C
Cebsocket is a lightweight websocket library for C.

## Usage
Usage is easy and simple. You can look to `examples/` directory. You can build examples with `make` command.

### Simple WebSocket Server
Here is an example for creating simple WebSocket server.

```C
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
```

## What about HTTP?
Cebsocket is designed to only handle WebSocket requests as a HTTP server. You can use it with Apache's [mod_proxy_ws_tunnel](https://httpd.apache.org/docs/current/mod/mod_proxy_wstunnel.html).

## What about SSL?
Also you can use Apache's [mod_proxy_ws_tunnel](https://httpd.apache.org/docs/current/mod/mod_proxy_wstunnel.html) for SSL.

## Build
Building is simple just do `make`.

```bash
make clean; make
```

You will see `websocket.o`. You can use it like:

```bash
gcc -o hello hello.c websocket.o
./hello
```

## Events
### Thread Safety
Event handler functions get called from client thread so you can make sure that they are **thread-safe**.

### `void on_connected(cebsocket_clients_t* client)`
Called when a `client` connected.

### `void on_disconnected(cebsocket_clients_t* client)`
Called when a `client` disconnected.

### `void on_message(cebsocket_clients_t* client, char* data)`
Called when a message is arrived from `client`.

## Functions
### `extern cebsocket_t* cebsocket_init(int port)`
Creates WebSocket server instance.

### `extern void cebsocket_listen(cebsocket_t* ws)`
Starts listening new connections.

### `extern void cebsocket_send(cebsocket_clients_t* client, char* message)`
Sends `message` to `client`.

## Types
### `cebsocket_t`
The WebSocket server instance.

```C
typedef struct cebsocket {
    int port;
    char* host_address;
    char* bind_address;
    cebsocket_clients_t* clients;
    void (*on_data)(cebsocket_clients_t*, char*);
    void (*on_connected)(cebsocket_clients_t*);
    void (*on_disconnected)(cebsocket_clients_t*);
} cebsocket_t;
```

### `cebsocket_clients_t`
The WebSocket client instance. It is also a linked-list.

```C
typedef struct cebsocket_clients {
    cebsocket_t* ws;
    int id;
    int socket;
    int server_socket;
    int address;
    char* ws_key;
    void* data;
    cebsocket_clients_t* prev;
    cebsocket_clients_t* next;
} cebsoket_clients_t;
```

## License
MIT