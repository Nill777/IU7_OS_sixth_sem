#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

struct stat statbuf;

int main()
{
    int fd1 = open("q.txt", O_RDWR);
    stat("q.txt", &statbuf);
    fprintf(stdout, "open fd1: inode = %ld, size = %ld bytes\n",
            statbuf.st_ino, statbuf.st_size);
    int fd2 = open("q.txt", O_RDWR);
    stat("q.txt", &statbuf);
    fprintf(stdout, "open fd2: inode = %ld, size = %ld bytes\n",
            statbuf.st_ino, statbuf.st_size);
    for (char c = 'a'; c <= 'z'; c++) {
        if (c % 2)
            write(fd1, &c, 1);
        else
            write(fd2, &c, 1);
        stat("q.txt", &statbuf);
        fprintf(stdout, "write: inode = %ld, size = %ld bytes\n",
                statbuf.st_ino, statbuf.st_size);
    }
    close(fd1);
    stat("q.txt", &statbuf);
    fprintf(stdout, "close fd1: inode = %ld, size = %ld bytes\n",
            statbuf.st_ino, statbuf.st_size);
    close(fd2);
    stat("q.txt", &statbuf);
    fprintf(stdout, "close fd2: inode = %ld, size = %ld bytes\n",
            statbuf.st_ino, statbuf.st_size);
    return 0;
}