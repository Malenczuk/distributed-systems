#ifndef TOKEN_H
#define TOKEN_H

#include <netinet/in.h>
#include <string>
#include <gtk/gtk.h>

typedef enum {
    FREE, DATA, CONFIRM, CONNECT, DISCONNECT, REMAP
} token_type;

typedef struct token {
    token_type type;
    unsigned char TTL;
    char destination[128];
    char source[128];
    char data[1024];
} Token;

struct swap {
    in_addr_t old_ip;
    in_port_t old_port;
    in_addr_t new_ip;
    in_port_t new_port;
};

Token empty_token();

Token create_token(token_type type, unsigned char TTL, char *source, char *destination, void *data, int len);

void print_token(token token);

#endif //TOKEN_H
