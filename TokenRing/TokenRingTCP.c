#include "TokenRing.c"
#include "TokenRingTCP.h"

void sigHandler() {
    FAILURE_EXIT(2, NULL, "\nSIGINT\n")
}

int main(int argc, char **argv) {
    init(argc, argv);

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

void handle_token(int socket) {
    token token = empty_token();
    struct sockaddr_in addr = recieve_token(socket, &token);

    if(!addr.sin_port)
        return;

    send_multicast(ID, strlen(ID));
    sleep(1);
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

void send_token(token token) {
    if (write(SOCKET[1], &token, sizeof(token)) != sizeof(token)) {
        FAILURE_EXIT(1, "write", "\nError : Could not send token\n")
    }
}

struct sockaddr_in recieve_token(int socket, token *token){
    struct sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    int addrlen = sizeof(addr);

    if ((read(socket, token, sizeof(*token))) != sizeof(*token)) {
        remove_socket(socket);
        return addr;
    }
    getpeername(socket, (struct sockaddr *) &addr, (socklen_t *) &addrlen);

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
            printf("Message wasn't delivered\n");
            token = empty_token();
        }
    }

    send_token(token);
}

void handle_confirm(token token) {
    if (strcmp(token.source, ID) == 0) {
        printf("Message was delivered successfully\n");
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

void remove_socket(int socket){
    if (epoll_ctl(epoll, EPOLL_CTL_DEL, socket, NULL) == -1){
        FAILURE_EXIT(1, "epoll_ctl", "\nError : Could not remove client's socket from epoll\n")
    }

    if (shutdown(socket, SHUT_RDWR) == -1) {
        FAILURE_EXIT(1, "shutdown", "\nError : Could not shutdown client's socket\n")
    }

    if (close(socket) == -1) {
        FAILURE_EXIT(1, "close", "\nError : Could not close client's socket\n")
    }
}

void connect_session() {
    if(SOCKET[1] != 0){
        if (shutdown(SOCKET[1], SHUT_RDWR) == -1)
            perror("shutdown");
        if (close(SOCKET[1]) == -1)
            perror("close");
        SOCKET[1] = 0;
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
    bzero(&swap, sizeof(swap));
    strcpy(swap.old_ip, OUT_IP);
    swap.old_port = OUT_PORT;
    swap.new_port = LOCAL_PORT;
    token token = create_token(CONNECT, 1, ID, ID, &swap, sizeof(swap));
    send_token(token);
}

void disconnect_tokenring() {
    struct swap swap;
    bzero(&swap, sizeof(swap));
    strcpy(swap.new_ip, OUT_IP);
    swap.new_port = OUT_PORT;
    swap.old_port = LOCAL_PORT;
    token token = create_token(DISCONNECT, 1, ID, ID, &swap, sizeof(swap));
    send_token(token);

    exit(1);
}

void init(int argc, char **argv){
    load_arguments(argc, argv);

    signal(SIGINT, sigHandler);

    if (atexit(del) == -1) FAILURE_EXIT(1, "atexit", "\nError : Could not set AtExit\n")

    queue = createQueue();

    init_multicast();

    init_TCP();

    init_epoll();

    connect_session();

    start_command_routine();
}

void init_TCP() {
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
}

void init_epoll(){
    struct epoll_event event;
    event.events = EPOLLIN | EPOLLPRI;

    if ((epoll = epoll_create1(0)) == -1) FAILURE_EXIT(1, "epoll_create1", "\nError : Could not create epoll\n");

    event.data.fd = -SOCKET[0];
    if (epoll_ctl(epoll, EPOLL_CTL_ADD, SOCKET[0], &event) == -1) {
        FAILURE_EXIT(1, "epoll_ctl", "\nError : Could not add IN Socket to epoll\n")
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