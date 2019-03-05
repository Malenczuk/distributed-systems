#ifndef TOKENRING_TOKENRING_H
#define TOKENRING_TOKENRING_H

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

void loop();

void handle_empty();

void handle_connect(token token, struct sockaddr_in addr);

void handle_disconnect(token token, struct sockaddr_in addr);

void handle_data(token token, struct sockaddr_in addr);

void handle_confirm(token token, struct sockaddr_in addr);

void handle_remap(token token, struct sockaddr_in addr);

void init_multicast();

void send_multicast(void *message, int size);

token empty_token();

token create_token(type type, unsigned char TTL, char *source, char *destination, void *data, int len);

void print_token(token token);

void tokenring_connect();

void tokenring_disconnect();

void init(int argc, char **argv);

void init_TCP();

void connect_TCP();

void *accept_TCP(void *x_void_ptr);

void init_UDP();

void send_token(token token);

struct sockaddr_in recieve_token(token *token);

void send_token_TCP(token token);

struct sockaddr_in recieve_token_TCP(token *token);

void send_token_UDP(token token);

struct sockaddr_in recieve_token_UDP(token *token);

#endif //TOKENRING_TOKENRING_H
