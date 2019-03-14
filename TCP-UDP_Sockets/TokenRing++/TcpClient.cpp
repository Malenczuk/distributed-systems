#include "TcpClient.h"
#include <unistd.h>
#include <string.h>
#include <sys/epoll.h>

TcpClient::TcpClient(char id[], in_port_t port, in_addr_t target_addr, in_port_t target_port, bool has_token)
        : Client(id, port, target_addr, target_port, has_token) {
    init();
}

void TcpClient::run() {
    connect_tokenring();
    if(hasToken) send_token(empty_token());

    epoll_event event{};
    while (1) {
        epoll_wait(epoll, &event, 1, -1);

        if (event.data.fd < 0)
            handle_new_connection(-event.data.fd);
        else
            handle_token(event.data.fd);

    }
}

void TcpClient::send_token(Token token) {
    write(sockets[1], &token,
          sizeof(token));
}

sockaddr_in TcpClient::recv_token(int socket, Token *token) {
    sockaddr_in addr{};
    bzero(&addr,
          sizeof(addr));
    int addrlen = sizeof(addr);

    if ((read(socket, token, sizeof(*token))) != sizeof(*token)) {
        remove_socket(socket);
        return addr;
    }
    getpeername(socket, (struct sockaddr *) &addr, (socklen_t *) &addrlen);

    return addr;
}

bool TcpClient::reconnect(token token) {
    if (Client::reconnect(token)) {
        connect_session();
        return true;
    }
    return false;
}

void TcpClient::init() {
    sockets[0] = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(localPort);

    bind(sockets[0], (struct sockaddr *) &addr, sizeof(addr));

    listen(sockets[0], 16);

    epoll_event event{};
    event.events = EPOLLIN | EPOLLPRI;
    event.data.fd = -sockets[0];

    epoll = epoll_create1(0);

    epoll_ctl(epoll, EPOLL_CTL_ADD, sockets[0], &event);

    connect_session();
}

void TcpClient::handle_new_connection(int socket) {
    int client = accept(socket, nullptr, nullptr);

    epoll_event event{};
    event.events = EPOLLIN | EPOLLPRI;
    event.data.fd = client;


    epoll_ctl(epoll, EPOLL_CTL_ADD, client, &event);
}

void TcpClient::connect_session() {
    if (sockets[1] != 0)
        destroy_socket(sockets[1]);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = targetAddr;
    addr.sin_port = ntohs(targetPort);

    sockets[1] = socket(AF_INET, SOCK_STREAM, 0);

    connect(sockets[1], (const struct sockaddr *) &addr, sizeof(addr));
}

void TcpClient::remove_socket(int socket) {
    epoll_ctl(epoll, EPOLL_CTL_DEL, socket, nullptr);
    destroy_socket(socket);
}

void TcpClient::destroy_socket(int socket) {
    shutdown(socket, SHUT_RDWR);
    close(socket);
}
