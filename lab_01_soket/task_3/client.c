#include <sys/types.h> 
#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include "calc.h"

#define SOCK_NAME "sock_serv.sv"

static int sock_fd;
struct sockaddr sockaddr = {.sa_family = AF_UNIX};

void sigint_handler() 
{
    unlink(sockaddr.sa_data);
    close(sock_fd);
    exit(0);
}

int main(void)
{
	sock_fd = socket(AF_UNIX, SOCK_DGRAM, 0);
	if  (sock_fd == -1)
	{
		perror("cant socket");
		exit(1);
	}
	
	struct sockaddr server_addr = {.sa_family = AF_UNIX};
	strcpy(server_addr.sa_data, SOCK_NAME);
	
	sprintf(sockaddr.sa_data, "%d.cl", getpid());
	
	if (bind(sock_fd, &sockaddr, sizeof(sockaddr)) == -1)
	{
		perror("cant bind");
		unlink(sockaddr.sa_data);
		close(sock_fd);
		exit(1);
	}	
	
	if (signal(SIGINT, sigint_handler) == SIG_ERR)
    {
		perror("cant signal");
		unlink(sockaddr.sa_data);
		close(sock_fd);
        exit(1);
    } 
	
	calc_t request;
	socklen_t addrlen;
	srand(time(NULL));
	while(1) {
		request.op = rand() % 4;
		request.arg1 = rand() % 100;
		request.arg2 = rand() % 100;
		
		if (sendto(sock_fd, &request, sizeof(request), 0, &server_addr, sizeof(server_addr)) == -1)
		{
			perror("cant send");
			unlink(sockaddr.sa_data);
			exit(1);
		}
		if (request.op == 0)
		printf("%lf + %lf = ", request.arg1, request.arg2);
		else if (request.op == 1)
		printf("%lf - %lf = ", request.arg1, request.arg2);
		else if (request.op == 2)
		printf("%lf * %lf = ", request.arg1, request.arg2);
		else
		printf("%lf / %lf = ", request.arg1, request.arg2);
		
		double res;
		if (recvfrom(sock_fd, &res, sizeof(res), 0, &server_addr, &addrlen) == -1) {
			perror("cant recvfrom");
			unlink(sockaddr.sa_data);
			close(sock_fd);
			continue;
		}
		printf("%lf\n", res);
		sleep(rand() % 3);
	}
	return 0;
}
