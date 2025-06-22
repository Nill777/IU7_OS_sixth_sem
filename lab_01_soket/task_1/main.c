#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>

#define MAXLEN_MSG 4
#define MSG_PR "aaa"
#define MSG_CH "111"

int main()
{
    int sk_fd[2];
    char buf[MAXLEN_MSG];
    buf[MAXLEN_MSG - 1] = '\0';
    pid_t pid;

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sk_fd) == -1) {
        perror("cant socketpair");
        exit(1);
    }

    if ((pid = fork()) == -1)
    {
        perror("cant fork");
        exit(1);
    }
    if (pid == 0)
    {
        close(sk_fd[1]);
        printf("child write: %s\n", MSG_PR);
        write(sk_fd[0], MSG_PR, MAXLEN_MSG);
        read(sk_fd[0], buf, MAXLEN_MSG);
        printf("child recieve: %s\n", buf);
        close(sk_fd[0]);
    }
    else {
        close(sk_fd[0]);
        read(sk_fd[1], buf, MAXLEN_MSG);
        printf("parent recieve: %s\n", buf);
        write(sk_fd[1], MSG_CH, MAXLEN_MSG);
        printf("parent write: %s\n", MSG_CH);
        close(sk_fd[1]);
    }
    return 0;
}
