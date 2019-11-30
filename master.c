#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>
#include <inttypes.h>

#include "shared.h"
#include "settings.h"

struct worker {
	int sock;
	int id;
	time_t done_time;
	weight_t real_weight;
	weight_t weight;
	int completed_tasks;
};

const char *algorithm_names[BALANCE__MAX] = {
	"No balancing",
	"Round robin",
	"Weighted least"
};

void assign_task(struct worker worker, struct task *tasks, int *task_offsets, int task_count, int worker_count, int *completed, int *assigned, enum balance_type balance_type);
void assign_round_robin_task(struct worker worker, struct task *tasks, int task_count, int worker_count, int *completed, int *assigned);
void read_input(int *primes_from, int *primes_to, int *task_size, int *worker_count, enum balance_type *balance_type);
void read_result(struct worker worker, result_t *results, int *completed);
uintmax_t *make_task_weights(struct task *tasks, int task_count, enum balance_type balance_type);
int *group_tasks(uintmax_t *task_weights, int task_count, struct worker *workers, int worker_count);
struct task *make_tasks(int primes_from, int primes_to, int task_size, int *task_count);
struct worker *accept_workers(int worker_count, weight_t *worker_weights, enum balance_type balance_type);
result_t load_balance(struct task *tasks, int task_count, int *task_offsets, struct worker *workers, int worker_count, enum balance_type balance_type);

void print_results(struct worker *workers, int number_of_workers, int number_of_primes, int primes_from, int primes_to, time_t total_time, enum balance_type algo);
void print_delimiter(int position);
void print_header(int number_of_workers, int completed_tasks, time_t total_time, enum balance_type algo);
void print_worker_result(struct worker worker, int work_number);

int main(void)
{
/*	int primes_from;
	int primes_to;
	int task_size;
	int worker_count; */
	int task_count;
	uintmax_t *task_weights;
	int *task_offsets;
	result_t result;
	time_t start_clock;
	time_t end_clock;
	struct task *tasks;
	struct worker *workers;
	struct settings *settings;

/*	read_input(&primes_from, &primes_to, &task_size, &worker_count, &balance_type); */

	settings = load_settings_file();
	settings_print(settings);

	/* settings doesn't read balance type yet, so set it to weighted for now */
	
	tasks = make_tasks(settings->task_limits.from, settings->task_limits.to,
			   settings->task_limits.task_number, &task_count);

	printf("Task count: %d\n", task_count);
	
	task_weights = make_task_weights(tasks, task_count, settings->balance_type);
	workers = accept_workers(settings->workers, settings->worker_weights,
				 settings->balance_type);
	task_offsets = group_tasks(task_weights, task_count, workers, settings->workers);

	start_clock = time(NULL);
	result = load_balance(tasks, task_count, task_offsets, workers,
			      settings->workers, settings->balance_type);
	end_clock = time(NULL);

	print_results(workers, settings->workers, result,
		      settings->task_limits.from,
		      settings->task_limits.to,
		      (end_clock - start_clock), settings->balance_type);

	free(settings->worker_weights);
	free(settings);
	return 0;
}

void read_input(int *primes_from, int *primes_to, int *task_size, int *worker_count, enum balance_type *balance_type)
{
	/*Only takes input, doesn't check it.*/
	printf("What type of balancing do you want? (0 = none, 1 = round robin, 2 = weighted least connections): ");
	scanf("%d", (int *)balance_type);
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
		tasks[i].from = (i * task_size) + 1;
		/*Makes sure the last task is not too big*/
		if (i == *task_count - 1) {
			tasks[i].to = primes_to;
		} else {
			tasks[i].to = (i + 1) * task_size;			
		}
	}
	return tasks;
	
}

uintmax_t *make_task_weights(struct task *tasks, int task_count, enum balance_type balance_type)
{
	int i;
	uintmax_t *task_weights = calloc(task_count, sizeof(uintmax_t));

	for (i = 0; i < task_count; i++) {
		if (balance_type == BALANCE_WEIGHTED) {
			/* complexity is n^2 */
			task_weights[i] = (tasks[i].to * tasks[i].to) - (tasks[i].from * tasks[i].from);
		} else {
			task_weights[i] = 1;
		}
	}


	return task_weights;
}

struct worker *accept_workers(int worker_count, weight_t *worker_weights, enum balance_type balance_type)
{
	int listen_socket;
	int i;
	int enable;
	struct worker *workers;
	struct sockaddr_in addr;
	char command[64];

	workers = malloc(sizeof(struct worker) * worker_count);
	if (workers == NULL) {
		printf("Malloc failed\n");
		exit(EXIT_FAILURE);
	}

	addr.sin_family = AF_INET;
	addr.sin_port = htons(LOADBALANCER_PORT);
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	
	listen_socket = socket(AF_INET, SOCK_STREAM, 0);
	enable = 1;
	setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
	bind(listen_socket, (struct sockaddr *)&addr, sizeof(addr));
	listen(listen_socket, 10); /*Max 10 in queue*/
	for (i = 0; i < worker_count; i++) {

		/* spawn a new worker as a seperate OS process */
		/* if fork returns 0 we are in a new child process */
		if (fork() == 0) {
			sprintf(command, "./worker %" PRIu8 "\n", worker_weights[i]);
			system(command);
			exit(EXIT_SUCCESS); /* exit the worker */
		}

		workers[i].completed_tasks = 0;
		workers[i].sock = accept(listen_socket, NULL, 0);
		workers[i].id = i;
		workers[i].done_time = -1;
		recv(workers[i].sock, &workers[i].real_weight, sizeof(weight_t), MSG_WAITALL);
		
		if (balance_type != BALANCE_WEIGHTED)
			workers[i].weight = 1;
		else
			workers[i].weight = workers[i].real_weight;
		
		printf("Accepted worker %d of %d\n", i+1, worker_count);
	}

	return workers;
	
}

int *group_tasks(uintmax_t *task_weights, int task_count, struct worker *workers, int worker_count)
{
	uintmax_t total_task_weight;
	uintmax_t total_worker_weight;
	int i;
	uintmax_t group_weight;
	uintmax_t task_weight_per_node_weight;
	int offset;
	
	int *offsets;

	offsets = malloc(worker_count * sizeof(int));

	for (i = 0, total_task_weight = 0; i < task_count; i++) {
		total_task_weight += task_weights[i];
	}

	for (i = 0, total_worker_weight = 0; i < worker_count; i++) {
		total_worker_weight += workers[i].weight;
	}

	task_weight_per_node_weight = total_task_weight / total_worker_weight;

	offset = 0;
	offsets[0] = 0;
	for (i = 1; i < worker_count; i++) {
		for (group_weight = 0; group_weight < (task_weight_per_node_weight * workers[i-1].weight); offset++) {
			group_weight += task_weights[offset];
		}
		offsets[i] = offset;
	}

	return offsets;
}

result_t load_balance(struct task *tasks, int task_count, int *task_offsets, struct worker *workers, int worker_count, enum balance_type balance_type)
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

	for (i = 0, nfds = 0; i < worker_count; i++) {
		if (workers[i].sock > nfds) {
			nfds = workers[i].sock;
		}
	}
	nfds++; /*Manual says to add one to nfds*/
	results = malloc(sizeof(result_t) * task_count);
	completed = calloc(task_count, sizeof(int));
	assigned = calloc(task_count, sizeof(int));

	for (i = 0; i < worker_count; i++) {
		if (balance_type == BALANCE_ROUND) {
			assign_round_robin_task(workers[i], tasks, task_count, worker_count,
						completed, assigned);
		} else {
			assign_task(workers[i], tasks, task_offsets, task_count,
				    worker_count, completed, assigned, balance_type);
		}
	}

	printf("Progress: ");
	completed_tasks = 0;
	while (completed_tasks < task_count) {
		FD_ZERO(&fd_set);
		for (i = 0; i < worker_count; i++) {
			FD_SET(workers[i].sock, &fd_set);
		}
		
		ready = select(nfds, &fd_set, NULL, NULL, NULL);
		for (; ready > 0; ready--) {
			for (i = 0; i < worker_count; i++) {
				if (FD_ISSET(workers[i].sock, &fd_set)) {
					FD_CLR(workers[i].sock, &fd_set);
					read_result(workers[i], results, completed);
					workers[i].done_time = time(NULL);
					if (balance_type == BALANCE_ROUND) {
						assign_round_robin_task(workers[i], tasks, task_count,
									worker_count, completed, assigned);
					} else {
						assign_task(workers[i], tasks, task_offsets, task_count,
							    worker_count, completed, assigned, balance_type);
					}
					completed_tasks++;
					workers[i].completed_tasks +=1;
				}
			}
		}

		if (completed_tasks % (task_count / 62) == 0) {
			printf("\u2588");
			fflush(stdout);
		}
	}

	printf("\n");
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
	/* printf("Got result from worker for task %d: %d\n", i, from_worker.result); */
}

void assign_task(struct worker worker, struct task *tasks, int *task_offsets, int task_count, int worker_count, int *completed, int *assigned, enum balance_type balance_type)
{
	int i;
	int end_offset;
	int task_offset;
	struct task *task;
	
	task = NULL;
	task_offset = task_offsets[worker.id];
	
	if (balance_type == BALANCE_NONE) {
		if (worker.id + 1 == worker_count)
			end_offset = task_count;
		else
			end_offset = task_offsets[worker.id+1];
	} else if (balance_type == BALANCE_WEIGHTED) {
		end_offset = task_count;
	} else {
		/* Noo, balance_type is wrong. This function should not be run with round robin */
		printf("Balance_type is not 0 1 or 2!\n");
		exit(EXIT_FAILURE);
	}
		
	
	for (i = task_offset; i < end_offset && task == NULL; i++) {
		if (!completed[i] && !assigned[i]) {
			task = &tasks[i];
			assigned[i] = 1;
			/* printf("Sending task %d (from %d to %d) to worker with weight %d\n",
			   i, task->from, task->to, worker.weight); */
		
		}
	}

	if (task == NULL && balance_type == BALANCE_WEIGHTED) {
		for (i = 0; i < task_offset && task == NULL; i++) {
			if (!completed[i] && !assigned[i]) {
				task = &tasks[i];
				assigned[i] = 1;
				/* printf("Sending task %d (from %d to %d) to worker %d with weight %d\n",
				   i, task->from, task->to, worker.id, worker.weight); */

			}
		}
	}

	if (task != NULL) {
		send(worker.sock, task, sizeof(struct task), 0);
	}
}

void print_delimiter(int position){
	int i;
	
	for (i = 0; i <= 72; i++) {
		if (position == 0) { /* top */
			if (i == 0)
				printf("\u250C");
			else if (i == 18 || i == 36 || i == 54)
				printf("\u252C");
			else if (i == 72)
				printf("\u2510");
			else
				printf("\u2500");
		} else if (position == 1) { /* middle */
			if (i == 0)				
				printf("\u251C");
			else if (i == 18 || i == 36 || i == 54)
				printf("\u253C");
			else if (i == 72)
				printf("\u2524");
			else
				printf("\u2500");
		} else if (position == 2) { /* end */
			if (i == 0)				
				printf("\u2514");
			else if (i == 18 || i == 36 || i == 54)
				printf("\u2534");
			else if (i == 72)
				printf("\u2518");
			else
				printf("\u2500");
		}
	}
	printf("\n");
}

void print_header(int number_of_workers, int completed_tasks, time_t total_time, enum balance_type algo){
	print_delimiter(0);
	printf("\u2502 %-15s \u2502 %-15s \u2502 %-15s \u2502 %-15s \u2502\n", "Workers", "Total tasks", "Total runtime", "Algorithm");
	print_delimiter(1);
	printf("\u2502 %15d \u2502 %15d \u2502 %11ld sec \u2502 %-15s \u2502\n", number_of_workers, completed_tasks, total_time, algorithm_names[algo]);
	print_delimiter(2);
}

void print_worker_result(struct worker worker, int work_number){
	time_t time_wasted;
	print_delimiter(1);

	time_wasted = time(NULL) - worker.done_time;
	printf("\u2502 %15d \u2502 %15d \u2502 %15d \u2502 %11ld sec \u2502\n", work_number, worker.real_weight, worker.completed_tasks, time_wasted);
}

void print_results(struct worker *workers, int number_of_workers, int number_of_primes, int primes_from, int primes_to, time_t total_time, enum balance_type algo) {
	int i;
	int completed_tasks;

	for (i = 0, completed_tasks = 0; i < number_of_workers; i++) {
		completed_tasks += workers[i].completed_tasks;
	}
	
	/* Total Header */
	print_header(number_of_workers, completed_tasks, total_time, algo);

	/* Spacing */
	printf("\n");
	print_delimiter(0);
	
	/* Work header */
	printf("\u2502 %-15s \u2502 %-15s \u2502 %-15s \u2502 %-15s \u2502\n", "Worker", "Weight", "Tasks Completed", "Time wasted");
	for ( i = 0; i < number_of_workers; i++){
		print_worker_result(workers[i], i);
	}

	/* Spacing */
	print_delimiter(2);
	printf("\n");
	print_delimiter(0);
	
	/* Prime result header */
	printf("\u2502 %-15s \u2502 %-15s \u2502 %-15s \u2502 %-15s \u2502\n", "Primes from", "Primes to", "Ints to check", "Result");
	print_delimiter(1);
	printf("\u2502 %15d \u2502 %15d \u2502 %15d \u2502 %15d \u2502\n", primes_from, primes_to, (primes_to - primes_from + 1), number_of_primes);
	
	print_delimiter(2);
}

void assign_round_robin_task(struct worker worker, struct task *tasks, int task_count, int worker_count, int *completed, int *assigned)
{
	int i;
	struct task *task;

	task = NULL;

	for (i = worker.id; i < task_count && task == NULL; i += worker_count) {
		if (!assigned[i] && !completed[i]) {
			task = &tasks[i];
			assigned[i] = 1;
			/*printf("Sending task %d (from %d to %d) to worker %d with weight %d\n",
			  i, task->from, task->to, worker.id, worker.weight);*/
		}
	}
	
	if (task != NULL) {
		send(worker.sock, task, sizeof(struct task), 0);
	}
}
