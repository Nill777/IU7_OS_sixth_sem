#include <sys/types.h> 
#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/un.h>
#include <pthread.h>
#include "calc.h"

#define SOCK_NAME "sock_serv.sv"

static int sock_fd;
pthread_mutex_t mutex;

typedef struct {
    calc_t *request;
    struct sockaddr *client_addr;
    socklen_t addrlen;
} thread_args_t;

void sigint_handler() 
{
    printf("shutdown server\n");
    unlink(SOCK_NAME);
    close(sock_fd);
    pthread_mutex_destroy(&mutex);
    exit(0);
}

void * handle_client(void *thread_args) {
    double result;
    thread_args_t *args = (thread_args_t *)thread_args;
    printf("recv message: %lf, %lf, %d\n", args->request->arg1, args->request->arg2, args->request->op);

    switch (args->request->op) {
        case 0:
            result = args->request->arg1 + args->request->arg2;
            break;
        case 1:
            result = args->request->arg1 - args->request->arg2;
            break;
        case 2:
            result = args->request->arg1 * args->request->arg2;
            break;
        case 3:
            if (args->request->arg2 != 0)
                result = args->request->arg1 / args->request->arg2;
            else {
                fprintf(stderr, "Error: division by zero\n");
                result = 0;
            }
            break;
        default:
            fprintf(stderr, "Error: unknown operation\n");
            result = 0;
            break;
    }

    pthread_mutex_lock(&mutex);
    if (sendto(sock_fd, &result, sizeof(result), 0, args->client_addr, args->addrlen) == -1)
        perror("cant sendto");
    pthread_mutex_unlock(&mutex);
    
    printf("send: %lf\n", result);

    return 0;
}

int main(void)
{
    sock_fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (sock_fd == -1)
    {
        perror("cant socket");
        exit(1);
    }
    
    struct sockaddr addr;
    addr.sa_family = AF_UNIX;
    strcpy(addr.sa_data, SOCK_NAME);
    
    if (bind(sock_fd, &addr, sizeof(addr)) == -1) 
    {
        perror("cant bind");
        close(sock_fd);
        exit(1);
    }
    
    if (signal(SIGINT, sigint_handler) == SIG_ERR)
    {
        perror("cant signal");
        unlink(SOCK_NAME);
        close(sock_fd);
        exit(1);
    }    
    
    struct sockaddr client_addr;
    socklen_t addrlen = sizeof(client_addr);
    calc_t request;
    pthread_t thread_id;
    thread_args_t thread_args;
    
    while(1) {
        if (recvfrom(sock_fd, &request, sizeof(request), 0, &client_addr, &addrlen) == -1) {
            perror("cant recvfrom");
            continue;
        }
        thread_args.request = &request;
        thread_args.client_addr = &client_addr;
        thread_args.addrlen = addrlen;

        handle_client(&thread_args);
    }
    return 0;
}
