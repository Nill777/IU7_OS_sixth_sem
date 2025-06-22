#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <netinet/in.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/select.h>
#include <string.h>
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

void sigint_handler()
{
    close(sfd);
    semctl(semid, 5, IPC_RMID, NULL);
    exit(EXIT_SUCCESS);
}

void handle_client_command(int client_fd, char *buf)
{
    unsigned index;
    int status;
    int full;

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
            if (send(client_fd, &buf, sizeof(buf), 0) == -1)
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
                printf("Client reserved letter %u\n", index);
            }
            else
            {
                if (arr[index] == ' ')
                {
                    status = ALREADLY_RESERVED;
                    printf("Client failed to reserve letter %u\n", index);
                }
                else
                {
                    status = ERROR;
                    printf("Server received invalid letter number %u\n", index);
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
                printf("All letters reserved. Shutting down server.\n");
                kill(getppid(), SIGINT);
            }

            if (send(client_fd, &status, sizeof(status), 0) == -1)
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

        default:
            printf("Unknown command: %c\n", buf[0]);
            break;
    }
}

int main(void)
{
    int listenfd, connfd;
    socklen_t clilen;
    struct sockaddr_in cliaddr, servaddr;
    fd_set read_fds, master_fds;
    int max_fd;
    char buf[BUF_SIZE];

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

    FD_ZERO(&master_fds);
    FD_SET(listenfd, &master_fds);
    max_fd = listenfd;

    while (1)
    {
        read_fds = master_fds;
        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) == -1)
        {
            perror("select");
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i <= max_fd; i++)
        {
            if (FD_ISSET(i, &read_fds))
            {
                if (i == listenfd)
                {
                    clilen = sizeof(cliaddr);
                    connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);
                    if (connfd == -1)
                    {
                        perror("accept");
                    }
                    else
                    {
                        FD_SET(connfd, &master_fds);
                        if (connfd > max_fd)
                        {
                            max_fd = connfd;
                        }
                        printf("New connection on socket %d\n", connfd);
                    }
                }
                else
                {
                    int nbytes = recv(i, buf, sizeof(buf), 0);
                    if (nbytes <= 0)
                    {
                        if (nbytes == 0)
                        {
                            printf("Socket %d hung up\n", i);
                        }
                        else
                        {
                            perror("recv");
                        }
                        close(i);
                        FD_CLR(i, &master_fds);
                    }
                    else
                    {
                        handle_client_command(i, buf); // Вызов функции для обработки команды
                    }
                }
            }
        }
    }

    if (semctl(semid, 5, IPC_RMID, NULL) == -1)
    {
        perror("semclt");
        exit(EXIT_FAILURE);
    }
}