#ifndef UDPCLIENT_H
#define UDPCLIENT_H

#include "Client.h"

class UdpClient : public Client{
public:
    UdpClient(char id[], in_port_t port, in_addr_t target_addr, in_port_t target_port, bool has_token);

    void run();

private:
    int h;
    int sockets[1];

    void init();

    void send_token(Token token) override;

    sockaddr_in recv_token(int socket, Token *token) override;

    bool reconnect(token token) override;
};


#endif //UDPCLIENT_H
