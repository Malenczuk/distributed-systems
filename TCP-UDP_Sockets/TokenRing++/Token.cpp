#include "Token.h"
#include <string.h>
#include <gtk/gtk.h>
#include <stdio.h>

Token empty_token() {
    Token token;
    bzero(&token, sizeof(token));
    return token;
}

Token create_token(token_type type, unsigned char TTL, char *source, char *destination, void *data, int len) {
    token token = empty_token();

    token.type = type;
    token.TTL = TTL;
    if (source)
        strcpy(token.source, source);
    if (destination)
        strcpy(token.destination, destination);
    if (data)
        memcpy(token.data, data, len);

    return token;
}

void print_token(token token) {
    printf("============[ TOKEN ]============\n");
    printf("TYPE : %d \t\tTTL : %d\n", token.type, token.TTL);
    printf("SRC  : '%s'\n", token.source);
    printf("DST  : '%s'\n", token.destination);
    printf("DATA : '");
    fwrite(token.data, 1024, 1, stdout);
    printf("'\n");
}