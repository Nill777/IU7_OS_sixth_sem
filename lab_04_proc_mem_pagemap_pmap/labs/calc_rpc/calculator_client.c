#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>
#include "calculator.h"

#define eps 10e-5

void calc_bakery_prog_1(char *host)
{
	CLIENT *clnt;
	double *calc_response;
	struct CALCULATOR calc_request;

#ifndef	DEBUG
	clnt = clnt_create (host, CALCBAKERY_PROG, CALCULATOR_VER, "udp");
	if (clnt == NULL) {
		clnt_pcreateerror (host);
		exit (1);
	}
#endif	/* DEBUG */

	while (true)
	{
		// Заполнение структуры запроса
		// calc_request.op = op;
		// calc_request.arg1 = arg1;
		// calc_request.arg2 = arg2;
		srand(time(NULL));
		calc_request.op = rand() % 3;
		calc_request.arg1 = rand() % 100;
		calc_request.arg2 = rand() % 100;
		calc_request.number = 0; // Номер клиента будет назначен сервером

		// Получение ID клиента
		int *client_id = calc_get_index_1(NULL, clnt);
		if (client_id == NULL)
		{
			clnt_perror(clnt, "RPC call failed");
			exit(1);
		}
		calc_request.number = *client_id;
		printf("Assigned client number: %d\n", calc_request.number); // Вывод ID клиента

		// Задержка перед отправкой запроса
		srand(time(NULL));
		sleep(5 + rand() % 5); // Случайная задержка

		// Выполнение RPC-вызова
		calc_request.number = *client_id;
		calc_response = calc_serv_1(&calc_request, clnt);
		if (calc_response == NULL)
		{
			clnt_perror(clnt, "RPC call failed or client arrived too late or invalid div");
			exit(1);
		}
		// printf("Result %lf", calc_response->result);
		// printf("%d", op);
		if (calc_request.op == 0)
			printf("client number %d %lf + %lf = %lf\n", calc_request.number, calc_request.arg1, calc_request.arg2, *calc_response);
		else if (calc_request.op == 1)
			printf("client number %d %lf - %lf = %lf\n", calc_request.number, calc_request.arg1, calc_request.arg2, *calc_response);
		else if (calc_request.op == 2)
			printf("client number %d %lf * %lf = %lf\n", calc_request.number, calc_request.arg1, calc_request.arg2, *calc_response);
		else
			printf("client number %d %lf \ %lf = %lf\n", calc_request.number, calc_request.arg1, calc_request.arg2, *calc_response);
	}

#ifndef	DEBUG
	clnt_destroy (clnt);
#endif	 /* DEBUG */
}


int
main (int argc, char *argv[])
{
	// if (argc != 5)
	// {
	// 	fprintf(stderr, "Usage: %s <server_host> <operation> <arg1> <arg2>\n", argv[0]);
	// 	fprintf(stderr, "Operations: 0=ADD, 1=SUB, 2=MUL, 3=DIV\n");
	// 	exit(1);
	// }
	if (argc != 2)
	{
		fprintf(stderr, "Usage: %s <server_host>\n", argv[0]);
		exit(1);
	}

	char *host = argv[1];
	// int op = atoi(argv[2]);
	// double arg1 = atof(argv[3]);
	// double arg2 = atof(argv[4]);

	// if (op < 0 || op > 3)
	// {
	// 	fprintf(stderr, "Invalid operation. Use 0=ADD, 1=SUB, 2=MUL, 3=DIV\n");
	// 	exit(1);
	// }

	// calc_bakery_prog_1(host, op, arg1, arg2);
	calc_bakery_prog_1(host);
	exit(0);
}
