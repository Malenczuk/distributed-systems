#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include "Client.h"

class TcpClient : public Client{
public:
    TcpClient(char id[], in_port_t port, in_addr_t target_addr, in_port_t target_port, bool has_token);

    void run() override;

private:
    int sockets[2]{};
    int epoll{};

    void send_token(Token token) override;

    sockaddr_in recv_token(int socket, Token *token) override;

    bool reconnect(token token) override;

    void init();

    void handle_new_connection(int socket);

    void connect_session();

    void remove_socket(int socket);

    void destroy_socket(int socket);
};


#endif //TCPCLIENT_H
