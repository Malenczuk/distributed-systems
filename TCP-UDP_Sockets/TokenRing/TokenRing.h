#ifndef TOKENRING_TOKENRING_H
#define TOKENRING_TOKENRING_H

#define MCAST_GRP "226.1.1.1"
#define MCAST_PORT 5007

#define FAILURE_EXIT(code, perr, format, ...) { if(format) fprintf(stdout, format, ##__VA_ARGS__); if(perr) perror(perr); exit(code);}

int MCAST_SOCKET = 0;
int SOCKET[2] = {0, 0};
pthread_t command;
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
struct Queue *queue;

typedef enum {
    EMPTY, CONNECT, DISCONNECT, DATA, CONFIRM, REMAP
} type;

const char* typeNames[] = {"EMPTY", "CONNECT", "DISCONNECT", "DATA", "CONFIRM", "REMAP" };

typedef struct {
    type token;
    unsigned char TTL;
    char destination[128];
    char source[128];
    char data[1024];
} token;

struct swap{
    char old_ip[16];
    uint16_t old_port;
    char new_ip[16];
    uint16_t new_port;
};

typedef struct QNode
{
    token token;
    struct QNode *next;
}QNode;

typedef struct Queue
{
    QNode *front, *rear;
}Queue;

QNode* newNode(token t);

Queue* createQueue();

void enQueue(Queue *q, token t);

QNode* deQueue(Queue *q);

void start_command_routine();

void *command_routine();

void init_multicast();

void send_multicast(void *message, int size);

token empty_token();

token create_token(type type, unsigned char TTL, char *source, char *destination, void *data, int len);

void print_token(token token);

void send_token(token token);

 #endif //TOKENRING_TOKENRING_H
