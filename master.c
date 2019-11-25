#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "shared.h"


struct worker {
	int sock;
	weight_t weight;
};

void assign_task(struct worker worker, struct task *tasks, int task_count, int *completed, int *assigned, int *task_weights);
void read_input(int *primes_from, int *primes_to, int *task_size, int *worker_count);
void read_result(struct worker worker, result_t *results, int *completed);
int *make_task_weights(struct task *tasks, int task_count);
struct task *make_tasks(int primes_from, int primes_to, int task_size, int *task_count);
struct worker *accept_workers(int worker_count);
result_t load_balance(struct task *tasks, int task_count, int *task_weights, struct worker *workers, int worker_count);

int main(void)
{
	int primes_from;
	int primes_to;
	int task_size;
	int worker_count;
	int task_count;
	int *task_weights;
	struct task *tasks;
	struct worker *workers;


	read_input(&primes_from, &primes_to, &task_size, &worker_count);
	tasks = make_tasks(primes_from, primes_to, task_size, &task_count);
	task_weights = make_task_weights(tasks, task_count);
	workers = accept_workers(worker_count);
	load_balance(tasks, task_count, task_weights, workers, worker_count);
	
	return 0;
}


void read_input(int *primes_from, int *primes_to, int *task_size, int *worker_count)
{
	/*Only takes input, doesn't check it.*/
	printf("Enter the range of numbers you want to check for primes (ex. 1 1000000): ");
	scanf("%d %d", primes_from, primes_to);
	printf("Enter task size (ex. 100): ");
	scanf("%d", task_size);
	printf("Enter amount of workers: ");
	scanf("%d", worker_count);
}

struct task *make_tasks(int primes_from, int primes_to, int task_size, int *task_count)
{
	struct task *tasks;
	int primes_delta;
	int i;
	

	primes_delta = primes_to - primes_from;
	*task_count = primes_delta / task_size;

	if ((primes_delta % task_size) != 0) {
		(*task_count)++;
	}

	tasks = malloc(sizeof(struct task) * *task_count);
	if (tasks == NULL) {
		printf("Malloc failed\n");
		exit(EXIT_FAILURE);
	}
	for (i = 0; i < *task_count; i++) {
		tasks[i].task_number = i;
		tasks[i].from = i * task_size;
		/*Makes sure the last task is not too big*/
		if (i == *task_count - 1) {
			tasks[i].to = primes_to;
		} else {
			tasks[i].to = (i + 1) * task_size - 1;			
		}
	}
	return tasks;
	
}

int *make_task_weights(struct task *tasks, int task_count)
{
	int i;
	int *task_weights = calloc(task_count, sizeof(int));

	for (i = 0; i < task_count; i++) {
		task_weights[i] = tasks[i].to;
	}
	
	return task_weights;
}

struct worker *accept_workers(int worker_count)
{
	int listen_socket;
	int i;
	struct worker *workers;
	struct sockaddr_in addr;

	workers = malloc(sizeof(struct worker) * worker_count);
	if (workers == NULL) {
		printf("Malloc failed\n");
		exit(EXIT_FAILURE);
	}

	addr.sin_family = AF_INET;
	addr.sin_port = htons(LOADBALANCER_PORT);
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	
	listen_socket = socket(AF_INET, SOCK_STREAM, 0);
	bind(listen_socket, (struct sockaddr *)&addr, sizeof(addr));
	listen(listen_socket, 10); /*Max 10 in queue*/
	for (i = 0; i < worker_count; i++) {
		workers[i].sock = accept(listen_socket, NULL, 0);
		recv(workers[i].sock, &workers[i].weight, sizeof(weight_t), MSG_WAITALL);
		printf("Accepted worker %d of %d\n", i+1, worker_count);
	}

	return workers;
	
}

result_t load_balance(struct task *tasks, int task_count, int *task_weights, struct worker *workers, int worker_count)
{
	int nfds;
	int i;
	int ready;
	int completed_tasks;
	int *completed;
	int *assigned;
	result_t total_primes;
	result_t *results;
	fd_set fd_set;
	
	FD_ZERO(&fd_set);
	for (i = 0, nfds = 0; i < worker_count; i++) {
		if (workers[i].sock > nfds) {
			nfds = workers[i].sock;
		}
		FD_SET(workers[i].sock, &fd_set);

	}
	nfds++; /*Manual says to add one to nfds*/
	results = malloc(sizeof(result_t) * task_count);
	completed = calloc(task_count, sizeof(int));
	assigned = calloc(task_count, sizeof(int));

	/* Start by giving each worker a task */
	
	completed_tasks = 0;
	while (completed_tasks < task_count) {
		ready = select(nfds, &fd_set, NULL, NULL, NULL);
		for (; ready > 0; ready--) {
			for (i = 0; i < worker_count; i++) {
				if (FD_ISSET(workers[i].sock, &fd_set)) {
					read_result(workers[i], results, completed);
					assign_task(workers[i], tasks, task_count, completed, assigned, task_weights);
					completed_tasks++;
				}
			}
		}
	}

	for (i = 0, total_primes = 0; i < task_count; i++) {
		total_primes += results[i];
	}
	return total_primes;
}

void read_result(struct worker worker, result_t *results, int *completed)
{
	struct result from_worker;
	int i;
	recv(worker.sock, &from_worker, sizeof(struct result), MSG_WAITALL);
	i = from_worker.task_number;
	results[i] = from_worker.result;
	completed[i] = 1;
	printf("Got result from worker for task %d: %d\n", i, from_worker.result);
}

void assign_task(struct worker worker, struct task *tasks, int task_count, int *completed, int *assigned, int *task_weights)
{
	int i;
	struct task *task;

	task = NULL;
	for (i = 0; i < task_count && task == NULL; i++) {
		if (!completed[i] && !assigned[i]) {
			assigned[i] = 1;
			task = &tasks[i];
		}
	}
	
	send(worker.sock, task, sizeof(struct task), 0);
}
