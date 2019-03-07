#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include "config.c"
#include "TokenRing.c"
#include "TokenRingUDP.h"

void sigHandler() {
    FAILURE_EXIT(2, NULL, "\nSIGINT\n")
}

int main(int argc, char **argv) {
    init(argc, argv);

    connect_tokenring();
    if (TOKEN) send_token(empty_token());

    handle_token();

    return 0;
}

void handle_token() {
    token token = empty_token();

    while (1) {
        struct sockaddr_in addr = recieve_token(&token);
        print_token(token);
        switch (token.token) {
            case EMPTY:
                handle_empty();
                break;
            case DATA:
                handle_data(token);
                break;
            case CONFIRM:
                handle_confirm(token);
                break;
            case REMAP:
                handle_remap(token);
                break;
            case CONNECT:
                handle_connect(token, addr);
                break;
            case DISCONNECT:
                handle_disconnect(token, addr);
                break;
        }
        sleep(1);
    }
}

void send_token(token token) {
    struct sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(OUT_IP);
    addr.sin_port = htons(OUT_PORT);

    if (sendto(SOCKET[0], &token, sizeof(token), 0, (const struct sockaddr *) &addr, sizeof(addr)) != sizeof(token)) {
        perror("write");
        exit(1);
    }
}

struct sockaddr_in recieve_token(token *token) {
    struct sockaddr_in addr;
    int addrlen = sizeof(addr);
    if (recvfrom(SOCKET[0], token, sizeof(*token), 0, (struct sockaddr *) &addr, (socklen_t *) &addrlen) !=
        sizeof(*token)) {
        perror("recvfrom");
        exit(1);
    }

    send_multicast(ID, strlen(ID));
    return addr;
}

void handle_empty() {
    pthread_mutex_lock(&queue_mutex);
    struct QNode *qNode = deQueue(queue);
    pthread_mutex_unlock(&queue_mutex);
    if(qNode){
        send_token(qNode->token);
        free(qNode);
    }
    else
        send_token(empty_token());
}

void handle_connect(token token, struct sockaddr_in addr) {
    struct swap *swap = (struct swap *) token.data;
    token.token = REMAP;
    strcpy(swap->new_ip, inet_ntoa(addr.sin_addr));

    if (!reconnect(token)){
        pthread_mutex_lock(&queue_mutex);
        enQueue(queue, token);
        pthread_mutex_unlock(&queue_mutex);
    }
}

void handle_disconnect(token token, struct sockaddr_in addr) {
    struct swap *swap = (struct swap *) token.data;
    token.token = REMAP;
    strcpy(swap->old_ip, inet_ntoa(addr.sin_addr));

    handle_remap(token);
}

void handle_data(token token) {
    if (strcmp(token.destination, ID) == 0) {
        print_token(token);
        token.token = CONFIRM;
    } else if (strcmp(token.source, ID) == 0) {
        if ((token.TTL -= 1) <= 0) {
            printf("TTL expired\n");
            token = empty_token();
        }
    }

    send_token(token);
}

void handle_confirm(token token) {
    if (strcmp(token.source, ID) == 0) {
        token = empty_token();
    }
    send_token(token);
}

void handle_remap(token token) {
    if(reconnect(token))
        send_token(empty_token());
    else
        send_token(token);
}

int reconnect(token token) {
    struct swap *swap = (struct swap *) token.data;
    if (!strcmp(OUT_IP, swap->old_ip) && OUT_PORT == swap->old_port) {
        strcpy(OUT_IP, swap->new_ip);
        OUT_PORT = swap->new_port;
        return 1;
    }
    return 0;
}

void connect_tokenring() {
    struct swap swap;
    bzero(&swap, sizeof(swap));
    strcpy(swap.old_ip, OUT_IP);
    swap.old_port = OUT_PORT;
    swap.new_port = LOCAL_PORT;
    token token = create_token(CONNECT, 1, ID, ID, &swap, sizeof(swap));
    send_token(token);
}

void disconnect_tokenring() {
    struct swap swap;
    strcpy(swap.new_ip, OUT_IP);
    swap.new_port = OUT_PORT;
    swap.old_port = LOCAL_PORT;
    token token = create_token(DISCONNECT, 1, ID, ID, &swap, sizeof(swap));
    send_token(token);

    exit(1);
}

void init(int argc, char **argv) {
    load_arguments(argc, argv);

    signal(SIGINT, sigHandler);

    if (atexit(del) == -1) FAILURE_EXIT(1, "atexit", "\nError : Could not set AtExit\n")

    queue = createQueue();

    init_multicast();

    init_UDP();

    start_command_routine();
}

void init_UDP() {
    if ((SOCKET[0] = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket");
        exit(1);
    }

    struct sockaddr_in addr;
    bzero(&addr, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(LOCAL_PORT);
    if (bind(SOCKET[0], (const struct sockaddr *) &addr, sizeof(addr)) == -1) {
        perror("bind");
        exit(1);
    }
}

void del() {
    pthread_cancel(command);
    if (queue)
        free(queue);
    if (close(SOCKET[0]) == -1)
        perror("close");
}
