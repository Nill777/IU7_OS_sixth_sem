#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#define SOCK_NAME "sock_serv.sv"
#define BUFFER_SIZE 64

int fd;
void sigalarm_handler() 
{
    unlink(SOCK_NAME);
    close(fd);
    printf("alarm shutdown server\n");
    exit(0);
}

int main() 
{
    fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (fd < 0)
    {
        perror("cant socket");
        exit(1);
    }
    struct sockaddr sockaddr = {.sa_family=AF_UNIX};
    strcpy(sockaddr.sa_data, SOCK_NAME);

    if (bind(fd, &sockaddr, sizeof(sockaddr)) == -1)
    {
        perror("cant bind");
        close(fd);
        exit(1);
    }

    if (signal(SIGALRM, sigalarm_handler) == SIG_ERR)
    {
        perror("cant signal");
        unlink(SOCK_NAME);
        close(fd);
        exit(1);
    } 

    char buf[BUFFER_SIZE];
    alarm(10);
    while(1)
    {
        int bytes_read = read(fd, buf, BUFFER_SIZE);
        if (bytes_read == -1) 
        {
            perror("cant read");
            close(fd);
            exit(1);
        }    
        else
        {
            buf[bytes_read] = '\0';
            printf("read message: %s\n", buf);
        }
    }
    return 0;
}
