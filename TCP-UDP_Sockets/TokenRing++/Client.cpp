#include "Client.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>

Client::Client(char id[], in_port_t port, in_addr_t target_addr, in_port_t target_port, bool has_token) {
    strcpy(ID, id);
    localPort = port;
    targetAddr = target_addr;
    targetPort = target_port;
    hasToken = has_token;
    printf("%d\n", has_token);
    printf("%d\n", hasToken);

    init_multicast();
}

void Client::init_multicast() {
    mcast_socket = socket(AF_INET, SOCK_DGRAM, 0);
}

void Client::send_multicast(void *message, int size) {
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(MCAST_GRP);
    addr.sin_port = htons(MCAST_PORT);

    sendto(mcast_socket, message, size, 0, (struct sockaddr *) &addr, sizeof(addr));
}

void Client::del_multicast() {
    close(mcast_socket);
}

void Client::connect_tokenring() {
    swap swap{};
    swap.old_ip = targetAddr;
    swap.old_port = targetPort;
    swap.new_port = localPort;
    token token = create_token(CONNECT, 1, ID, ID, &swap, sizeof(swap));
    send_token(token);
}

void Client::handle_token(int socket) {
    Token token;
    struct sockaddr_in addr = recv_token(socket, &token);

    if (!addr.sin_port)
        return;

    send_multicast(ID, strlen(ID));
    sleep(1);
    switch (token.type) {
        case FREE:
            handle_free();
            break;
        case DATA:
            handle_data(token);
            break;
        case CONFIRM:
            handle_confirm(token);
            break;
        case CONNECT:
            handle_connect(token, addr);
            break;
        case DISCONNECT:
            handle_disconnect(token, addr);
            break;
        case REMAP:
            handle_remap(token);
            break;
    }
}

void Client::handle_free() {
    if (!queue.empty()) {
        Token token = queue.front();
        queue.pop();
        send_token(token);
    } else {
        send_token(empty_token());
    }
}

void Client::handle_data(token token) {
    if (strcmp(token.destination, ID) == 0) {
        if(show_message)
            show_message(token);
        print_token(token);
        token.type = CONFIRM;
    } else if (strcmp(token.source, ID) == 0) {
        if ((token.TTL -= 1) <= 0) {
            printf("Message wasn't delivered\n");
            token = empty_token();
        }
    }

    send_token(token);
}

void Client::handle_confirm(token token) {
    if (strcmp(token.source, ID) == 0) {
        printf("Message was delivered successfully\n");
        token = empty_token();
    }
    send_token(token);
}

void Client::handle_connect(token token, struct sockaddr_in addr) {
    swap *swap = (struct swap *) token.data;
    token.type = REMAP;
    swap->new_ip = addr.sin_addr.s_addr;

    if (!reconnect(token)) {
        queue.push(token);
    }
}

void Client::handle_disconnect(token token, struct sockaddr_in addr) {
    swap *swap = (struct swap *) token.data;
    token.type = REMAP;
    swap->old_ip = addr.sin_addr.s_addr;

    handle_remap(token);
}

void Client::handle_remap(token token) {
    if (reconnect(token))
        send_token(empty_token());
    else
        send_token(token);
}

bool Client::reconnect(token token) {
    swap *swap = (struct swap *) token.data;
    if ((targetAddr == swap->old_ip) && (targetPort == swap->old_port)) {
        targetAddr = swap->new_ip;
        targetPort = swap->new_port;
        return true;
    }
    return false;
}