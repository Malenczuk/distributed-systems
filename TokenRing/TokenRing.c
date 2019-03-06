#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include "TokenRing.h"
#include "config.c"

int MCAST_SOCKET;
int SOCKET[2];
pthread_t accept_thread = NULL;

token messagess[16];
int msgs = 0;
int end = -1;

void sigHandler() {
    end = 1;
    if(accept_thread)
        pthread_cancel(accept_thread);
}

int remap(token token) {
    struct swap *swap = (struct swap *) token.data;
    if (!strcmp(OUT_IP, swap->old_ip) && OUT_PORT == swap->old_port) {
        strcpy(OUT_IP, swap->new_ip);
        OUT_PORT = swap->new_port;
        return 1;
    }
    return 0;
}

int main(int argc, char **argv) {
    print_token(create_token(CONFIRM, 12, "Ala", "xD", "Ala ma Kota!", strlen("Ala ma Kota!")));
    init(argc, argv);

    tokenring_connect();
    if (TOKEN) send_token(empty_token());

    sleep(1);
    loop();

    return 0;
}

void loop() {
    token token = empty_token();

    while (1) {
        struct sockaddr_in addr = recieve_token(&token);
        print_token(token);
        switch (token.token) {
            case EMPTY:
                handle_empty();
                break;
            case DATA:
                handle_data(token, addr);
                break;
            case CONFIRM:
                handle_confirm(token, addr);
                break;
            case REMAP:
                handle_remap(token, addr);
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

void handle_empty() {
    token token = empty_token();

    if (end == -1) {
        signal(SIGTSTP, sigHandler);
        atexit(sigHandler);
        end = 0;
    }

    if (msgs > 0) {
        token = messagess[0];
        for (int i = 1; i <= msgs; ++i) {
            messagess[i - 1] = messagess[i];
        }
    }
    if (end == 1) tokenring_disconnect();
    send_token(token);
}

void handle_connect(token token, struct sockaddr_in addr) {
    struct swap *swap = (struct swap *) token.data;
    token.token = REMAP;
    strcpy(swap->new_ip, inet_ntoa(addr.sin_addr));

    if (!remap(token))
        memcpy(&messagess[msgs++], &token, sizeof(token));
}

void handle_disconnect(token token, struct sockaddr_in addr) {
    struct swap *swap = (struct swap *) token.data;
    token.token = REMAP;
    strcpy(swap->old_ip, inet_ntoa(addr.sin_addr));

    handle_remap(token, addr);
}

void handle_data(token token, struct sockaddr_in addr) {
    if (strcmp(token.destination, ID) == 0) {
        token.token = CONFIRM;
    } else if (strcmp(token.source, ID) == 0) {
        if ((token.TTL -= 1) <= 0) {
            printf("TTL expired\n");
            bzero(&token, sizeof(token));

        }
    }

    send_token(token);
}

void handle_confirm(token token, struct sockaddr_in addr) {
    if (strcmp(token.source, ID) == 0) {
        token = empty_token();
    }
    send_token(token);
}

void handle_remap(token token, struct sockaddr_in addr) {
    if(remap(token))
        send_token(empty_token());
    else
        send_token(token);
}

token empty_token() {
    token token;
    bzero(&token, sizeof(token));
    return token;
}

token create_token(type type, unsigned char TTL, char *source, char *destination, void *data, int len) {
    token token = empty_token();

    token.token = type;
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
    printf("TYPE : %s \t\tTTL : %d\n", typeNames[token.token], token.TTL);
    printf("SRC  : '%s'\n", token.source);
    printf("DST  : '%s'\n", token.destination);
    printf("DATA : '");
    fwrite(token.data, 1025, 1, stdout);
    printf("'\n");
}

void tokenring_connect() {
    if(PROTOCOL == TCP)
        connect_TCP();

    struct swap swap;
    strcpy(swap.old_ip, OUT_IP);
    swap.old_port = OUT_PORT;
    swap.new_port = LOCAL_PORT;
    token token = create_token(CONNECT, 1, ID, ID, &swap, sizeof(swap));
    send_token(token);
}

void tokenring_disconnect() {
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

    init_multicast();

    switch (PROTOCOL) {
        case UDP:
            init_UDP();
            break;
        case TCP:
            init_TCP();
            break;
        default:
            printf("Unexpected error with selected protocol\n");
            exit(1);
    }
}

void init_TCP() {
    if ((SOCKET[0] = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
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

    if (listen(SOCKET[0], 16)) {
        perror("listen");
        exit(1);
    }

    pthread_create(&accept_thread, NULL, accept_TCP, NULL);

    if ((SOCKET[1] = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(1);
    }

    bzero(&addr, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = 0;
    if (bind(SOCKET[1], (const struct sockaddr *) &addr, sizeof(addr)) == -1) {
        perror("bind");
        exit(1);
    }
}

void *accept_TCP(void *x_void_ptr){
    while(1) {
        if(SOCKET[0] = (SOCKET[0], NULL, NULL) == -1){
            perror("accept");
            exit(1);
        }
    }
}

void connect_TCP(){
    printf("Connecting");

    struct sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(OUT_IP);
    addr.sin_port = htons(OUT_PORT);
    if(connect(SOCKET[1], (const struct sockaddr *) &addr, sizeof(addr)) != 0 ){
        perror("connect");
        exit(1);
    }
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

void send_token(token token) {
    switch (PROTOCOL) {
        case UDP:
            send_token_UDP(token);
            break;
        case TCP:
            send_token_TCP(token);
            break;
        default:
            printf("Unexpected error with selected protocol\n");
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

void send_token_TCP(token token) {
    if(send(SOCKET[1], &token, sizeof(token), 0) != sizeof(token)) {
        perror("send");
        exit(1);
    }
}

void send_token_UDP(token token) {
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

void init_multicast() {
    if ((MCAST_SOCKET = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket");
        exit(1);
    }
}

void send_multicast(void *message, int size) {
    struct sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(MCAST_GRP);
    addr.sin_port = htons(MCAST_PORT);

    if ((sendto(MCAST_SOCKET, message, size, 0, (struct sockaddr *) &addr, sizeof(addr))) < 0) {
        perror("sendto");
        exit(1);
    }
}