#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <popt.h>
#include "config.c"

#define MCAST_GRP "224.1.1.1"
#define MCAST_PORT 5007
int MCAST_SOCKET;

typedef enum {
    EMPTY, CONNECT, DISCONNECT, DATA, COPIED, REMAP
} type;

typedef struct {
    type token;
    char TTL;
    char destination[128];
    char source[128];
    char data[1024];
} token;

//char ID[] = "Elaa";
//uint16_t LOCAL_PORT = 4002;
//char OUT_IP[] = "127.0.0.1";
//uint16_t OUT_PORT = 4001;

int SOCKET;

token messagess[16];
int msgs = 0;
int end = -1;


void send_multicast(void *message, int size) {
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(MCAST_GRP);
    addr.sin_port = htons(MCAST_PORT);

    if ((sendto(MCAST_SOCKET, message, size, 0, (struct sockaddr *) &addr, sizeof(addr))) < 0) {
        perror("sendto");
        exit(1);
    }
}

void send_token(token token) {
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(OUT_IP);
    addr.sin_port = htons(OUT_PORT);

    if (sendto(SOCKET, &token, sizeof(token), 0, (const struct sockaddr *) &addr, sizeof(addr)) != sizeof(token)) {
        perror("write");
        exit(1);
    }
}

struct sockaddr_in recieve_token(token *token) {
    struct sockaddr_in addr;
    int addrlen = sizeof(addr);
    if (recvfrom(SOCKET, token, sizeof(*token), 0, (struct sockaddr *) &addr, (socklen_t *) &addrlen) !=
        sizeof(*token)) {
        perror("recvfrom");
        exit(1);
    }
    return addr;
}

void init_multicast() {
    if ((MCAST_SOCKET = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket");
        exit(1);
    }
}

void init() {
    if ((SOCKET = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket");
        exit(1);
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(LOCAL_PORT);
    if (bind(SOCKET, (const struct sockaddr *) &addr, sizeof(addr)) == -1) {
        perror("bind");
        exit(1);
    }
}

void conn() {
    token token;
    token.token = CONNECT;
    token.TTL = 1;
    strcpy(token.source, ID);
    strcpy(token.destination, ID);
    sprintf(token.data, "%d %hu", inet_addr(OUT_IP), OUT_PORT);

    send_token(token);
}

void disconn() {
    token token;
    token.token = DISCONNECT;
    token.TTL = 1;
    strcpy(token.source, ID);
    strcpy(token.destination, ID);
    sprintf(token.data, "%d %hu", inet_addr(OUT_IP), OUT_PORT);

    send_token(token);
    exit(1);
}

void sigHandler() {
    end = 1;
}

int remap(token token) {
    uint16_t old_port, new_port;
    uint32_t old_ip;
    struct in_addr new_ip;
    sscanf(token.data, "%d %hu %d %hu", &old_ip, &old_port, &new_ip.s_addr, &new_port);
    if (inet_addr(OUT_IP) == old_ip && OUT_PORT == old_port) {
        strcpy(OUT_IP, inet_ntoa(new_ip));
        OUT_PORT = new_port;
        return 1;
    }
    return 0;
}

int main(int argc, char **argv) {

    load_arguments(argc, argv);

    init_multicast();
    send_multicast(ID, strlen(ID));

    init();

    conn();
    if (TOKEN) {
        token token;
        memset(&token, 0, sizeof(token));
        token.token = DATA;
        token.TTL = 2;
        strcpy(token.source, ID);
        strcpy(token.destination, "Leszek");
        strcpy(token.data, "Ala Ma Kota");

        send_token(token);
        printf("sent\n");
    }
    while (1) {
        char buffer[128];
        token token;
        bzero(&token, sizeof(token));

        struct sockaddr_in addr = recieve_token(&token);
        switch (token.token) {
            case EMPTY:
                if (end == -1){
                    signal(SIGTSTP, sigHandler);
                    atexit(sigHandler);
                    end = 0;
                }
                printf("\n");
                if (msgs > 0) {
                    token = messagess[0];
                    for (int i = 1; i <= msgs; ++i) {
                        messagess[i - 1] = messagess[i];
                    }
                }
                if(end == 1) disconn();
                send_token(token);
                break;
            case DATA:
                if (strcmp(token.destination, ID) == 0) {
                    printf("token: %d\tTTL:%d\ndest: %s\nsrc: %s\ndata: %s\n", token.token, token.TTL,
                           token.destination, token.source, token.data);
                    token.token = COPIED;
                } else if (strcmp(token.source, ID) == 0) {
                    if ((token.TTL -= 1) <= 0) {
                        printf("TTL expired\n");
                        bzero(&token, sizeof(token));

                    }
                }

                send_token(token);
                break;
            case COPIED:
                printf("message delivered");
                if (strcmp(token.source, ID) == 0) {
                    bzero(&token, sizeof(token));
                }
                send_token(token);
                break;
            case REMAP:
                printf("token: %d\tTTL:%d\ndest: %s\nsrc: %s\ndata: %s\n", token.token, token.TTL,
                       token.destination, token.source, token.data);
                remap(token);
                bzero(&token, sizeof(token));
                send_token(token);
                break;
            case CONNECT:
                printf("token: %d\tTTL:%d\ndest: %s\nsrc: %s\ndata: %s\n", token.token, token.TTL,
                       token.destination, token.source, token.data);
                token.token = REMAP;
                sprintf(buffer, " %d %hu", addr.sin_addr.s_addr, ntohs(addr.sin_port));
                strcat(token.data, buffer);
                printf("token: %d\tTTL:%d\ndest: %s\nsrc: %s\ndata: %s\n", token.token, token.TTL,
                       token.destination, token.source, token.data);
                if(!remap(token))
                    memcpy(&messagess[msgs++], &token, sizeof(token));
                break;
            case DISCONNECT:
                printf("token: %d\tTTL:%d\ndest: %s\nsrc: %s\ndata: %s\n", token.token, token.TTL,
                       token.destination, token.source, token.data);
                token.token = REMAP;
                sprintf(buffer, "%d %hu ", addr.sin_addr.s_addr, ntohs(addr.sin_port));
                strcat(buffer, token.data);
                strcpy(token.data, buffer);
                printf("token: %d\tTTL:%d\ndest: %s\nsrc: %s\ndata: %s\n", token.token, token.TTL,
                       token.destination, token.source, token.data);
                remap(token);

                bzero(&token, sizeof(token));
                send_token(token);
                break;

        }
        sleep(1);
    }


    return 0;
}
