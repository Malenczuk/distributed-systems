#include "config.c"
#include "TokenRing.h"

QNode* newNode(token t)
{
    struct QNode *temp = (struct QNode*)malloc(sizeof(struct QNode));
    temp->token = t;
    temp->next = NULL;
    return temp;
}

Queue *createQueue()
{
    struct Queue *q = (struct Queue*)malloc(sizeof(struct Queue));
    q->front = q->rear = NULL;
    return q;
}

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

QNode *deQueue(struct Queue *q)
{
    if (q->front == NULL)
        return NULL;

    struct QNode *temp = q->front;
    q->front = q->front->next;

    if (q->front == NULL)
        q->rear = NULL;
    return temp;
}

void start_command_routine() {
    // Start Commander Thread
    if (pthread_create(&command, NULL, command_routine, NULL) != 0) {
        FAILURE_EXIT(1, "pthread_create", "\nError : Could not create Commander Thread\n")
    }
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
