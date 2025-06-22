#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include "calculator.h"

#define MAX_CLIENTS 1000
#define TIMEOUT_SEC 1

int client_number[MAX_CLIENTS] = {0};
bool choosing[MAX_CLIENTS] = {false};
int last_num = 0;

int *
calc_get_index_1_svc(void *argp, struct svc_req *rqstp)
{
	static int result;
	int i = 0;
	choosing[i] = true;

	// Генерация уникального клиента
	int max_num = 0;
	for (int j = 0; j < MAX_CLIENTS; j++)
	{
		if (client_number[j] > max_num)
			max_num = client_number[j];
	}
	client_number[i] = ++max_num;
	result = client_number[i];

	choosing[i] = false;

	// printf("Assigned client ID: %d\n", result);  // Вывод ID клиента на сервере
	return &result;
}

double *
calc_serv_1_svc(struct CALCULATOR *argp, struct svc_req *rqstp)
{
	static double result;
	int ret_num = argp->number;
	// printf("Received operation: %d, arg1: %f, arg2: %f\n", argp->op, argp->arg1, argp->arg2);

	// Случайная задержка перед обработкой запроса
	// srand(time(NULL));
	// sleep(1 + rand() % 3);  // Задержка от 1 до 3 секунд
	result = ret_num;
	int i = ret_num;
	time_t start, end;
	for (int j = 0; j < MAX_CLIENTS; j++)
	{
		// while (choosing[j])
		// 	;
		if (last_num > client_number[i])
		{
			client_number[i] = 0;
			// result = -1;
			return NULL;
		}
		start = clock();
		while ((client_number[j] > 0) && (client_number[j] < client_number[i] || client_number[j] == client_number[i] && j < i))
		{
			end = clock();
			if ((end - start) / CLOCKS_PER_SEC > TIMEOUT_SEC)
				break;
		}
	}

	// Выполнение операции
	switch (argp->op)
	{
	case ADD:
		result = argp->arg1 + argp->arg2;
		break;
	case SUB:
		result = argp->arg1 - argp->arg2;
		break;
	case MUL:
		result = argp->arg1 * argp->arg2;
		break;
	case DIV:
		if (argp->arg2 == 0)
			return NULL;
		result = argp->arg1 / argp->arg2;
		break;
	default:
		break;
	}

	last_num = client_number[i];
	client_number[i] = 0;
	for (int j = 0; j < MAX_CLIENTS; j++)
		if (client_number[j] > 0)
			return &result;
	last_num = 0;
	// client_number[ret_num] = 0;  // Очистка ID клиента после завершения
	// result.return_number = ret_num;
	return &result;
}
