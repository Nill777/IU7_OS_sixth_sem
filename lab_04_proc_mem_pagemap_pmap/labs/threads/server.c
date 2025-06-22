#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <netinet/in.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <pthread.h>

#include "common.h"

int sfd, semid;

char arr[ARR_SIZE];

#define READER_QUEUE 0
#define WRITER_QUEUE 1
#define READER 2
#define WRITER 3
#define SHADOW_WRITER 4
struct sembuf start_rd[] = {{READER_QUEUE, 1, 0},
                            {WRITER_QUEUE, 0, 0},
                            {SHADOW_WRITER, 0, 0},
                            {READER, 1, 0},
                            {READER_QUEUE, -1, 0}};

struct sembuf stop_rd[] = {{READER, -1, 0}};

struct sembuf start_wr[] = {{WRITER_QUEUE, 1, 0},
                            {READER, 0, 0},
                            {WRITER, -1, 0},
                            {SHADOW_WRITER, 1, 0},
                            {WRITER_QUEUE, -1, 0}};

struct sembuf stop_wr[] = {{SHADOW_WRITER, -1, 0},
                           {WRITER, 1, 0}};

void* process_client(void *args)
{
    unsigned index;
    int connfd = *((int *)args);
    char buf[BUF_SIZE];
    int status;
    int full;

    printf("[%d] || Got new connection!\n", getpid());

    while (1)
    {
        if (recv(connfd, buf, BUF_SIZE, 0) <= 0)
        {
            printf("[%d] || Server finished\n", getpid());
            break;
        }


        switch (buf[0])
        {
            case 'r':
                if (semop(semid, start_rd, 5) == -1)
                {
                    perror("semop");
                    exit(EXIT_FAILURE);
                }
                for (size_t i = 0; i < ARR_SIZE; i++)
                {
                    buf[i] = arr[i];
                }
                if (send(connfd, &buf, sizeof(buf), 0) == -1)
                {
                    perror("error send");
                    exit(EXIT_FAILURE);
                }

                if (semop(semid, stop_rd, 1) == -1)
                {
                    perror("semop");
                    exit(EXIT_FAILURE);
                }

                break;

            case 'w':
                index = (int) buf[1];
                if (semop(semid, start_wr, 5) == -1)
                {
                    perror("semop");
                    exit(EXIT_FAILURE);
                }

                if (index < ARR_SIZE && arr[index] != ' ')
                {
                    arr[index] = ' ';
                    status = OK;
                    printf("[%d] || Client reserved letter %u\n", getpid(), index);
                }
                else
                {
                    if (arr[index] == ' ')
                    {
                        status = ALREADLY_RESERVED;
                        printf("[%d] || Client failed to reserve letter %u\n", getpid(), index);
                    }
                    else
                    {
                        status = ERROR;
                        printf("[%d] || Server received invalid letter number %u\n", getpid(), index);
                    }
                }

                full = 1;
                for (size_t i = 0; full && i < ARR_SIZE; i++)
                {
                    if (arr[i] != ' ')
                        full = 0;
                }

                if (full)
                {
                    printf("[%d] || All letters reserved. Shutting down server.\n", getpid());
                    kill(getppid(), SIGINT); // Signal parent process to shutdown
                }

                if (send(connfd, &status, sizeof(status), 0) == -1)
                {
                    perror("error send");
                    exit(EXIT_FAILURE);
                }

                if (semop(semid, stop_wr, 2) == -1)
                {
                    perror("semop");
                    exit(EXIT_FAILURE);
                }
                break;
        }
    }

    sleep(100);
    return NULL;
}

void sigint_handler()
{
    close(sfd);
    semctl(semid, 5, IPC_RMID, NULL);
    exit(EXIT_SUCCESS);
}

int main(void)
{
    int listenfd, connfd;
    socklen_t clilen;
    struct sockaddr_in cliaddr, servaddr;
    pthread_t th;

    if (signal(SIGINT, sigint_handler) == (void *)-1)
    {
        perror("cannot set handler");
        exit(EXIT_FAILURE);
    }

    semid = semget(IPC_PRIVATE, 5, IPC_CREAT | 0666);
    if (semid == -1)
        perror("semget");
    if (semctl(semid, READER_QUEUE, SETVAL, 0) == -1)
        perror("semctl");
    if (semctl(semid, WRITER_QUEUE, SETVAL, 0) == -1)
        perror("semctl");
    if (semctl(semid, READER, SETVAL, 0) == -1)
        perror("semctl");
    if (semctl(semid, WRITER, SETVAL, 1) == -1)
        perror("semctl");
    if (semctl(semid, SHADOW_WRITER, SETVAL, 0) == -1)
        perror("semctl");

    for (size_t i = 0; i < ARR_SIZE; i++)
        arr[i] = (char)('a' + i);

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd == -1)
    {
        perror("error socket");
        exit(EXIT_FAILURE);
    }

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(SERV_PORT);

    if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1)
    {
        perror("error bind");
        exit(EXIT_FAILURE);
    }

    if (listen(listenfd, 1024) == -1)
    {
        perror("error listen");
        exit(EXIT_FAILURE);
    }

    printf("listening on port %d\n", SERV_PORT);

    while (1)
    {
        clilen = sizeof(cliaddr);
        connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);

        if (pthread_create(&th, NULL, process_client, &connfd) != 0)
        {
            perror("pthread_create");
            exit(EXIT_FAILURE);
        }

        if (pthread_detach(th))
        {
            perror("pthread_detach");
            exit(EXIT_FAILURE);
        }
    }

    if (semctl(semid, 5, IPC_RMID, NULL) == -1)
    {
        perror("semclt");
        exit(EXIT_FAILURE);
    }
}
