#ifndef TOKENRING_TOKENRINGUDP_H
#define TOKENRING_TOKENRINGUDP_H

void handle_token();

void send_token(token token);

struct sockaddr_in recieve_token(token *token);

void handle_empty();

void handle_connect(token token, struct sockaddr_in addr);

void handle_disconnect(token token, struct sockaddr_in addr);

void handle_data(token token);

void handle_confirm(token token);

void handle_remap(token token);

int reconnect(token token);

void connect_tokenring();

void disconnect_tokenring();

void init(int argc, char **argv);

void init_UDP();

void del();

#endif //TOKENRING_TOKENRINGUDP_H
