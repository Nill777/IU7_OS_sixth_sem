#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "common.h"

void request(int sockfd)
{
    int index;
    char buf[BUF_SIZE];
    buf[0] = 'r';
    if (send(sockfd, &buf, sizeof(buf), 0) == -1)
    {
        perror("error send");
        exit(1);
    }

    if (recv(sockfd, &buf, sizeof(buf), 0) == -1)
    {
        perror("error recieve");
        exit(1);
    }

    printf("Recieved letters: ");
    for (size_t i = 0; i < ARR_SIZE; i++)
        printf("%c ", buf[i]);
    puts("");

    index = -1;
    for (int i = 0; i < ARR_SIZE; i++)
    {
        if (buf[i] != ' ')
        {
            index = i;
            break;
        }
    }

    if (index == -1)
    {
        printf("No availible letters left! Disconnecting from server\n");
        exit(1);
    }

    usleep(500000 + rand() % 500000);

    buf[0] = 'w';
    buf[1] = (char) index;

    printf("Sending index %d to server\n", index);

    if (send(sockfd, &buf, sizeof(buf), 0) == -1)
    {
        perror("error send");
        exit(1);
    }

    if (recv(sockfd, &buf, sizeof(buf), 0) == -1)
    {
        perror("error recieve");
        exit(1);
    }

    switch ((int) buf[0])
    {
        case OK:
            puts("Response: OK\n");
            break;
        case ALREADLY_RESERVED:
            puts("Response: already reserved\n");
            break;
        default:
            perror("Unexpected response from server");
            exit(1);
    }
    usleep(2000000 + rand() % 1500000);
}

int main(int argc, char **argv)
{
    int sockfd;
    struct sockaddr_in servaddr;

    if (argc != 2)
    {
        perror("usage: tcpcli <IPaddress>");
        exit(1);
    }

    while (1) {
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd == -1)
        {
            perror("error socket");
            exit(1);
        }

        servaddr.sin_family = AF_INET;
        servaddr.sin_port = htons(SERV_PORT);
        if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) == -1)
        {
            perror("inet pton");
            exit(1);
        }

        if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1)
        {
            perror("connection error");
            exit(1);
        }
        else
            printf("Connected\n");

        request(sockfd);
        close(sockfd);
    }

    exit(0);
}
