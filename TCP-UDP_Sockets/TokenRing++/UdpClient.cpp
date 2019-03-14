#include "UdpClient.h"

UdpClient::UdpClient(char id[], in_port_t port, in_addr_t target_addr, in_port_t target_port, bool has_token)
        : Client(id, port, target_addr, target_port, has_token) {
    init();
}

void UdpClient::run() {
    connect_tokenring();

    if(hasToken) send_token(empty_token());

    while (1) {
        printf("%d %d %d\n", localPort, targetAddr, targetPort);
        handle_token(sockets[0]);
    }
}

void UdpClient::init() {
    sockets[0] = socket(AF_INET, SOCK_DGRAM, 0);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(localPort);

    bind(sockets[0], (struct sockaddr *) &addr, sizeof(addr));
}

void UdpClient::send_token(Token token) {
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = targetAddr;
    addr.sin_port = htons(targetPort);

    sendto(sockets[0], &token, sizeof(Token), 0, (const struct sockaddr *) &addr, sizeof(addr));
}

sockaddr_in UdpClient::recv_token(int socket, Token *token) {
    sockaddr_in addr{};
    int addr_len = sizeof(addr);
    recvfrom(socket, token, sizeof(Token), 0, (struct sockaddr *) &addr, (socklen_t *) &addr_len);

    return addr;
}

bool UdpClient::reconnect(token token) {
    return Client::reconnect(token);
}