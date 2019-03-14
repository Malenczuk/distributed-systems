#ifndef CLIENT_H
#define CLIENT_H

#include <netinet/in.h>
#include <queue>
#include "Token.h"

#define MCAST_GRP "226.1.1.1"
#define MCAST_PORT 5007

class Client {
public:
    void (*show_message)(Token) = nullptr;

    std::queue<Token> queue;

    virtual void run() = 0;

protected:
    bool hasToken;
    int mcast_socket{};
    char ID[128]{};
    in_port_t localPort;
    in_addr_t targetAddr{};
    in_port_t targetPort{};

    Client(char id[], in_port_t port, in_addr_t target_addr, in_port_t target_port, bool has_token);

    void init_multicast();

    void send_multicast(void *message, int size);

    void del_multicast();

    void connect_tokenring();

    virtual void send_token(Token token) = 0;

    virtual sockaddr_in recv_token(int socket, Token *token) = 0;

    void handle_token(int socket);

    void handle_free();

    void handle_data(token token);

    void handle_confirm(token token);

    void handle_connect(token token, struct sockaddr_in addr);

    void handle_disconnect(token token, struct sockaddr_in addr);

    void handle_remap(token token);

    virtual bool reconnect(token token);
};


#endif //CLIENT_H
