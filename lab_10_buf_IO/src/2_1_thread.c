#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

struct stat statbuf;

struct thread_arg {
    int fd;
    int i;
};

void* thread_start(void* arg)
{
    struct thread_arg* targ = arg;

    for (char c = 'a'; c <= 'z'; c++)
        if (c % 2 == targ->i) {
            write(targ->fd, &c, 1);
            fstat(targ->fd, &statbuf);
            off_t size = lseek(targ->fd, 0, SEEK_CUR);
            fprintf(stdout, "write %d: inode number = %ld, size = %ld bytes\n",
                    targ->i, statbuf.st_ino, size);
        }

    return NULL;
}

int main()
{
    int fd[2] = {
        open("q.txt", O_RDWR),
        open("q.txt", O_RDWR)
    };
    pthread_t thr[2];
    struct thread_arg targ[2];

    for (int i = 0; i < 2; i++) 
    {
        targ[i].fd = fd[i];
        targ[i].i = i;

        if (pthread_create(&thr[i], NULL, thread_start, &targ[i])) 
        {
            perror("pthread_create");
            return 1;
        }
    }

    for (int i = 0; i < 2; i++)
        if (pthread_join(thr[i], NULL)) {
            perror("pthread_join");
            return 1;
        }

    close(fd[0]);
    close(fd[1]);

    return 0;
}