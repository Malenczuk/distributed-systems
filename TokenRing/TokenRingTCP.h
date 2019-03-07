#ifndef TOKENRING_TOKENRINGTCP_H
#define TOKENRING_TOKENRINGTCP_H

#define MCAST_GRP "224.1.1.1"
#define MCAST_PORT 5007

void handle_token(int socket);

void send_token(token token);

struct sockaddr_in recieve_token(int socket, token *token);

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

void init(int argc, char **argv);

void init_TCP();

void init_epoll();

void del();

#endif //TOKENRING_TOKENRINGTCP_H
