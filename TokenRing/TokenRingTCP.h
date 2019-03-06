#ifndef TOKENRING_TOKENRINGTCP_H
#define TOKENRING_TOKENRINGTCP_H

#define MCAST_GRP "224.1.1.1"
#define MCAST_PORT 5007

typedef enum {
    EMPTY, CONNECT, DISCONNECT, DATA, CONFIRM, REMAP
} type;

const char* typeNames[] = {"EMPTY", "CONNECT", "DISCONNECT", "DATA", "CONFIRM", "REMAP" };

typedef struct {
    type token;
    unsigned char TTL;
    char destination[128];
    char source[128];
    char data[1024];
} token;

struct swap{
    char old_ip[16];
    uint16_t old_port;
    char new_ip[16];
    uint16_t new_port;
};

token empty_token();

token create_token(type type, unsigned char TTL, char *source, char *destination, void *data, int len);

void print_token(token token);

void send_token(token token);

void handle_token(int socket);

void handle_empty();

void handle_connect(token token, struct sockaddr_in addr);

void handle_disconnect(token token, struct sockaddr_in addr);

void handle_data(token token);

void handle_confirm(token token);

void handle_remap(token token);

int reconnect(token token);

void handle_connection(int socket);

void connect_session();

void connect_tokenring();

void init();

void del();

void init_multicast();

void send_multicast(void *message, int size);

#endif //TOKENRING_TOKENRINGTCP_H
