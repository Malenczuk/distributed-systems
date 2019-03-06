#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <libnet.h>
#include <pthread.h>
#include <sys/epoll.h>
#include "config.c"
#include "TokenRingTCP.h"

#define FAILURE_EXIT(code, perr, format, ...) { if(format) fprintf(stdout, format, ##__VA_ARGS__); if(perr) perror(perr); exit(code);}

struct QNode
{
    token token;
    struct QNode *next;
};

// The queue, front stores the front node of LL and rear stores ths
// last node of LL
struct Queue
{
    struct QNode *front, *rear;
};

// A utility function to create a new linked list node.
struct QNode* newNode(token t)
{
    struct QNode *temp = (struct QNode*)malloc(sizeof(struct QNode));
    temp->token = t;
    temp->next = NULL;
    return temp;
}

// A utility function to create an empty queue
struct Queue *createQueue()
{
    struct Queue *q = (struct Queue*)malloc(sizeof(struct Queue));
    q->front = q->rear = NULL;
    return q;
}

// The function to add a key k to q
void enQueue(struct Queue *q, token t)
{
    // Create a new LL node
    struct QNode *temp = newNode(t);

    // If queue is empty, then new node is front and rear both
    if (q->rear == NULL)
    {
        q->front = q->rear = temp;
        return;
    }

    // Add the new node at the end of queue and change rear
    q->rear->next = temp;
    q->rear = temp;
}

// Function to remove a key from given queue q
struct QNode *deQueue(struct Queue *q)
{
    // If queue is empty, return NULL.
    if (q->front == NULL)
        return NULL;

    // Store previous front and move front one node ahead
    struct QNode *temp = q->front;
    q->front = q->front->next;

    // If front becomes NULL, then change rear also as NULL
    if (q->front == NULL)
        q->rear = NULL;
    return temp;
}

pthread_t command;
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
int MCAST_SOCKET;
int SOCKET[2] = {0, 0};
int epoll;
struct Queue *queue;

void sigHandler() {
    FAILURE_EXIT(2, NULL, "\nSIGINT\n")
}

int main(int argc, char **argv) {
    if (atexit(del) == -1) FAILURE_EXIT(1, "atexit", "\nError : Could not set AtExit\n")

    load_arguments(argc, argv);
    init();
    connect_session();
    connect_tokenring();

    if(TOKEN) send_token(empty_token());

    struct epoll_event event;
    while (1) {
        if (epoll_wait(epoll, &event, 1, -1) == -1) FAILURE_EXIT(1, "epoll_wait", "\nError : epoll_wait failed\n")

        if (event.data.fd < 0)
            handle_connection(-event.data.fd);
        else
            handle_token(event.data.fd);

    }
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
    fwrite(token.data, 1024, 1, stdout);
    printf("'\n");
}

void send_token(token token) {
    if (write(SOCKET[1], &token, sizeof(token)) != sizeof(token)) {
        FAILURE_EXIT(1, "write", "\nError : Could not send token\n")
    }
}

void handle_token(int socket) {
    sleep(1);
    token token;
    if (read(socket, &token, sizeof(token)) != sizeof(token)) {
        FAILURE_EXIT(1, "read", "\nError : Could not read token\n")
    }
    struct sockaddr_in addr;
    int addrlen = sizeof(addr);
    getpeername(socket, (struct sockaddr *) &addr, (socklen_t *) &addrlen);

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
        connect_session();
        return 1;
    }
    return 0;
}

void handle_connection(int socket) {
    int client;
    if ((client = accept(socket, NULL, NULL)) == -1) {
        FAILURE_EXIT(1, "accept", "\nError : Could not accept new client\n")
    }

    struct epoll_event event;
    event.events = EPOLLIN | EPOLLPRI;
    event.data.fd = client;

    if (epoll_ctl(epoll, EPOLL_CTL_ADD, client, &event) == -1) {
        FAILURE_EXIT(1, "epoll_ctl", "\nError : Could not add new client to epoll\n")
    }
}

void connect_session() {
    if(SOCKET[1] != 0){
        if (shutdown(SOCKET[1], SHUT_RDWR) == -1)
            perror("shutdown");
        if (close(SOCKET[1]) == -1)
            perror("close");
    }

    struct sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(OUT_IP);
    addr.sin_port = ntohs(OUT_PORT);

    if ((SOCKET[1] = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        FAILURE_EXIT(1, "socket", "\nError : Could not create OUT socket\n")
    }

    if (connect(SOCKET[1], (const struct sockaddr *) &addr, sizeof(addr)) == -1) {
        FAILURE_EXIT(1, "connect", "\nError : Could not connect to IN socket\n")
    }
}

void connect_tokenring() {
    struct swap swap;
    strcpy(swap.old_ip, OUT_IP);
    swap.old_port = OUT_PORT;
    swap.new_port = LOCAL_PORT;
    token token = create_token(CONNECT, 1, ID, ID, &swap, sizeof(swap));
    send_token(token);
}

void *command_routine() {
    token token;
    strcpy(token.source, ID);
    token.token = DATA;
    token.TTL = 2;
    while (1) {
        printf("Enter Destination: \n");
        fgets(token.destination, 128, stdin);
        token.destination[strlen(token.destination) - 1] = '\0';
        printf("Enter Message: \n");
        fgets(token.data, 1024, stdin);
        token.data[strlen(token.data) - 1] = '\0';
        pthread_mutex_lock(&queue_mutex);
        enQueue(queue, token);
        pthread_mutex_unlock(&queue_mutex);
    }
}

void init() {
    // Handle Signal
    signal(SIGINT, sigHandler);

    // Create queue
    queue = createQueue();

    // Init IN Socket
    struct sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(LOCAL_PORT);

    if ((SOCKET[0] = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        FAILURE_EXIT(1, "socket", "\nError : Could not create IN socket\n")
    }

    if (bind(SOCKET[0], (const struct sockaddr *) &addr, sizeof(addr)) == -1) {
        FAILURE_EXIT(1, "bind", "\nError : Could not bind IN socket\n")
    }

    if (listen(SOCKET[0], 64) == -1) {
        FAILURE_EXIT(1, "listen", "\nError : Could not listen to IN socket\n")
    }

    // Init EPoll
    struct epoll_event event;
    event.events = EPOLLIN | EPOLLPRI;

    if ((epoll = epoll_create1(0)) == -1) FAILURE_EXIT(1, "epoll_create1", "\nError : Could not create epoll\n");

    event.data.fd = -SOCKET[0];
    if (epoll_ctl(epoll, EPOLL_CTL_ADD, SOCKET[0], &event) == -1) {
        FAILURE_EXIT(1, "epoll_ctl", "\nError : Could not add IN Socket to epoll\n")
    }

    // Start Commander Thread
    if (pthread_create(&command, NULL, command_routine, NULL) != 0) {
        FAILURE_EXIT(1, "pthread_create", "\nError : Could not create Commander Thread\n")
    }
}

void del() {
    pthread_cancel(command);
    if (queue)
        free(queue);
    if (close(SOCKET[0]) == -1)
        perror("close");
    if (shutdown(SOCKET[1], SHUT_RDWR) == -1)
        perror("shutdown");
    if (close(SOCKET[1]) == -1)
        perror("close");
    if (close(epoll) == -1)
        perror("close");
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