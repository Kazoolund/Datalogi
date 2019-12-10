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
#include <signal.h>

#include "shared.h"
#include "settings.h"

struct worker {
	int sock; /* The socket used to communicate with the worker */
	int id;
	pid_t pid; /* The process id of the subprocess */
	struct timespec done_time; /* The time that the worker was done working, and started doing nothing */
	weight_t real_weight; /* The weight the worker says it has */
	weight_t weight; /* The weight the algorithm uses */
	int completed_tasks; /* The amount of tasks the worker has done */
};

/* Function prototypes */
int *group_tasks(uintmax_t *task_weights, int task_count, struct worker *workers, int worker_count);
void assign_task(struct worker worker, struct task *tasks, int *task_offsets, int task_count,
		 int worker_count, int *completed, int *assigned, enum balance_type balance_type);
void assign_round_robin_task(struct worker worker, struct task *tasks, int task_count, int worker_count, int *completed, int *assigned);
void read_input(int *primes_from, int *primes_to, int *task_size, int *worker_count, enum balance_type *balance_type);
void read_result(struct worker worker, result_t *results, int *completed);
void print_results(struct worker *workers, int number_of_workers, int number_of_primes, int primes_from, int primes_to, double total_time, enum balance_type algo);
void print_delimiter(int position);
void print_header(int number_of_workers, int completed_tasks, double total_time, enum balance_type algo);
void print_worker_result(struct worker worker, int work_number);
result_t load_balance(struct task *tasks, int task_count, int *task_offsets, struct worker *workers, int worker_count, enum balance_type balance_type);
uintmax_t *make_task_weights(struct task *tasks, int task_count, enum balance_type balance_type);
struct task *make_tasks(int primes_from, int primes_to, int task_size, int *task_count);
struct worker *accept_workers(int worker_count, weight_t *worker_weights, enum balance_type balance_type);

int main(int argc, char *argv[])
{
	int task_count;
	int i;
	double total_time_used;
	uintmax_t *task_weights;
	int *task_offsets;
	result_t result;
	struct timespec start_clock;
	struct timespec end_clock;
	struct task *tasks;
	struct worker *workers;
	struct settings *settings;

	if (argc != 2) {
		printf("Usage: %s settings-file.txt\n", argv[0]);
		exit(EXIT_SUCCESS);
	}

	/* Read the settings for the computation from the file specified in argv[1] */
	settings = load_settings_file(argv[1]);

	/* Print the settings to be sure it was read correctly */
	settings_print(settings);

	/* Construct an array of task based on the input settings */
	tasks = make_tasks(settings->task_limits.from, settings->task_limits.to,
			   settings->task_limits.task_number, &task_count);
	printf("Task count: %d\n", task_count);

	/* Construct an array with the weight of each task, based on the balance_type (none and round robin ignores weights) */
	task_weights = make_task_weights(tasks, task_count, settings->balance_type);

	/* Start the correct amount of workers with the given worker weights as subprocesses so they run in parallel */
	workers = accept_workers(settings->workers, settings->worker_weights,
				 settings->balance_type);

	/* Calculate task offsets, based on the amount of tasks, the weight of the tasks, and the weight of the workers.
	 * Each offset is an index into the task array that each worker has to start getting tasks from, to ensure
	 * that they get tasks that fit their weights. */
	task_offsets = group_tasks(task_weights, task_count, workers, settings->workers);

	/* Get the time before the actual balancing happens, then do the balancing and then get the time again */
	clock_gettime(CLOCK_MONOTONIC, &start_clock);
	result = load_balance(tasks, task_count, task_offsets, workers,
			      settings->workers, settings->balance_type);
	clock_gettime(CLOCK_MONOTONIC, &end_clock);

	total_time_used = timespec_to_double(end_clock) - timespec_to_double(start_clock);

	/* Finally, print the results and information about the balancing in a nice table */
	print_results(workers, settings->workers, result,
		      settings->task_limits.from,
		      settings->task_limits.to,
		      total_time_used, settings->balance_type);

	/* kill all the worker processes */
	for (i = 0; i < settings->workers; i++) {
		kill(workers[i].pid, SIGKILL);
	}

	/* Do some cleanup */
	free(tasks);
	free(task_weights);
	free(task_offsets);
	free(workers);
	free(settings->worker_weights);
	free(settings);
	return 0;
}


/* make_tasks creates an array of struct task, with tasks of the size specified
 * in task_size. All the tasks together cover the numbers in the range primes_from
 * to primes_to. The total task count is set in the output parameter task_count. */
struct task *make_tasks(int primes_from, int primes_to, int task_size, int *task_count)
{
	struct task *tasks;
	int primes_delta;
	int i;

	/* calculate how many numbers to cover, and how many tasks it gives */
	primes_delta = primes_to - primes_from;
	*task_count = primes_delta / task_size;

	/* If the amount of numbers is not divisible by task_size, we add an extra task */
	if ((primes_delta % task_size) != 0) {
		(*task_count)++;
	}

	/* Allocate memory and check for failure */
	tasks = malloc(sizeof(struct task) * *task_count);
	if (tasks == NULL) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}

	/* Create the tasks in a loop */
	for (i = 0; i < *task_count; i++) {
		tasks[i].task_number = i;
		tasks[i].from = (i * task_size) + primes_from;

		/* Make sure the last task is not too big by setting the upper
		 * limit to primes_to */
		if (i == *task_count - 1) {
			tasks[i].to = primes_to;
		} else {
			tasks[i].to = (i + 1) * task_size;			
		}
	}
	return tasks;
}

/* make_task_weights creates an array of task weights based on their complexity and
 * the loadbalancer type in use. The none and round robin balancers sets the weight
 * to 1 because they don't use it. */
uintmax_t *make_task_weights(struct task *tasks, int task_count, enum balance_type balance_type)
{
	int i;
	uintmax_t *task_weights;

	/* Allocate memory and check for failure */
	task_weights = calloc(task_count, sizeof(uintmax_t));
	if (task_weights == NULL) {
		perror("calloc");
		exit(EXIT_FAILURE);
	}

	/* Calculate the weight for each task in a loop */
	for (i = 0; i < task_count; i++) {
		if (balance_type == BALANCE_WEIGHTED) {
			/* complexity is n^2, so we calculate the weight based on that */
			task_weights[i] = (tasks[i].to * tasks[i].to) - (tasks[i].from * tasks[i].from);
		} else {
			task_weights[i] = 1;
		}
	}

	return task_weights;
}

/* accept_workers spawns worker_count subprocesses each running the worker program with a weight from
 * worker_weights. The workers then connect via sockets, and the sockets are saved together with the worker
 * weights in an array of worker structs. The none and round robin loadbalancers ignores the weight. */
struct worker *accept_workers(int worker_count, weight_t *worker_weights, enum balance_type balance_type)
{
	int listen_socket;
	int i;
	int enable;
	pid_t forkres;
	struct worker *workers;
	struct sockaddr_in addr;
	char command[64];

	/* Allocate memory and check for failure */
	workers = malloc(sizeof(struct worker) * worker_count);
	if (workers == NULL) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}

	/* Setup the address struct that specifies where the socket will bind itself to */
	addr.sin_family = AF_INET;
	addr.sin_port = htons(LOADBALANCER_PORT);
	addr.sin_addr.s_addr = inet_addr(LOADBALANCER_IP);

	/* Create the socket */
	listen_socket = socket(AF_INET, SOCK_STREAM, 0);

	/* Set the SO_REUSEADDR socket option, so we can run the program faster without waiting
	 * for the operating system to release the socket address. */
	enable = 1;
	setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));

	/* Bind the socket to localhost so the workers can find it */
	bind(listen_socket, (struct sockaddr *)&addr, sizeof(addr));

	/* Start listening for incomming connections. A maximum of worker_count incomming connections
	 * is allowed */
	listen(listen_socket, worker_count);

	for (i = 0; i < worker_count; i++) {
		/* spawn a new worker as a seperate OS process.
		 * If fork returns 0 we are in a new child process, and the code in the if-statement is run */
		forkres = fork();
		if (forkres == 0) {
			/* Prepare a command string to be executed by the system, and execute it */
			sprintf(command, "./worker %" PRIu8 "\n", worker_weights[i]);
			system(command);
			exit(EXIT_SUCCESS); /* exit the worker process after the worker program is done */
		} else if (forkres == -1) {
			perror("fork");
			exit(EXIT_FAILURE);
		}


		/* Accept worker number i and fill in the rest of the worker struct */
		workers[i].sock = accept(listen_socket, NULL, 0);
		workers[i].completed_tasks = 0;
		workers[i].id = i;
		workers[i].pid = forkres;
		clock_gettime(CLOCK_MONOTONIC, &workers[i].done_time);
		/* the weight is received as the first thing from the worker */
		recv(workers[i].sock, &workers[i].real_weight, sizeof(weight_t), MSG_WAITALL);

		/* If we are not using the weighted least connections algorithm, the effective weight is set to 1,
		 * otherwise it is set to what was received from the worker. */
		if (balance_type != BALANCE_WEIGHTED)
			workers[i].weight = 1;
		else
			workers[i].weight = workers[i].real_weight;
		
		printf("Accepted worker %d of %d\n", i+1, worker_count);
	}

	close(listen_socket);
	return workers;
}

/* group_tasks creates an array of integers, that is to be used as offsets into the task array.
 * Each worker gets an offset, and the spacing between the offsets tries to make sure that each
 * worker gets a chunk of the tasks that fits both the weights of the task in that chunk, and
 * the weight of the worker */
int *group_tasks(uintmax_t *task_weights, int task_count, struct worker *workers, int worker_count)
{
	uintmax_t total_task_weight;
	uintmax_t total_worker_weight;
	int i;
	uintmax_t group_weight;
	uintmax_t task_weight_per_node_weight;
	int offset;
	
	int *offsets;

	/* Allocate memory and check for failure */
	offsets = malloc(worker_count * sizeof(int));
	if (offsets == NULL) {
		perror("malloc");
		exit(EXIT_SUCCESS);
	}

	/* Sum up the total task weight */
	for (i = 0, total_task_weight = 0; i < task_count; i++) {
		total_task_weight += task_weights[i];
	}

	/* Sum up the total worker weight */
	for (i = 0, total_worker_weight = 0; i < worker_count; i++) {
		total_worker_weight += workers[i].weight;
	}

	/* calculate how much task_weight each unit of worker weight should get */
	task_weight_per_node_weight = total_task_weight / total_worker_weight;

	/* Calculate the offsets, starting with offset 0 for the first worker. Each group should
	 * have a total weight equal to the worker weight times the task_weight_per_node_weight.
	 * Because there is no guarantee that the tasks split up precisely, the last group usually
	 * ends up larger than the others */
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

/* The load_balance function runs a loop where it delegates tasks to workers, and gets results back, until all tasks are completed.
 * Based on the balance_type parameter, the function performs the delegation of tasks differently, but it should always return the same result,
 * which is the sum of the results of all the tasks. */
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

	/* We create an fd_set, which allows us to monitor all worker sockets for incomming data.
	 * Before we can create the fd_set, we need to know what the biggets socket is, and add one to that. */
	for (i = 0, nfds = 0; i < worker_count; i++) {
		if (workers[i].sock > nfds) {
			nfds = workers[i].sock;
		}
	}
	nfds++; /* Manual for select() says to add one to nfds */

	/* Allocate memory and check for failure */
	results = malloc(sizeof(result_t) * task_count);
	if (results == NULL) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}

	/* the completed and assigned array keeps track of which tasks are completed and assigned */
	/* Allocate memory and check for failure */
	completed = calloc(task_count, sizeof(int));
	if (completed == NULL) {
		perror("calloc");
		exit(EXIT_FAILURE);
	}

	/* Allocate memory and check for failure */
	assigned = calloc(task_count, sizeof(int));
	if (assigned == NULL) {
		perror("calloc");
		exit(EXIT_FAILURE);
	}

	/* Start out by assigning each worker one task to get things going */
	for (i = 0; i < worker_count; i++) {
		if (balance_type == BALANCE_ROUND) {
			assign_round_robin_task(workers[i], tasks, task_count, worker_count,
						completed, assigned);
		} else {
			assign_task(workers[i], tasks, task_offsets, task_count,
				    worker_count, completed, assigned, balance_type);
		}
	}

	printf("Progress: "); /* print the start of the progress bar */

	/* Run the get result and assign loop until all tasks are completed*/
	completed_tasks = 0;
	while (completed_tasks < task_count) {
		/* Re-initialize the fd_set */
		FD_ZERO(&fd_set);
		for (i = 0; i < worker_count; i++) {
			FD_SET(workers[i].sock, &fd_set);
		}

		/* wait for data to become ready on any of the worker sockets */
		ready = select(nfds, &fd_set, NULL, NULL, NULL);

		/* the ready variable specifies how many workers were ready, so loop over them */
		for (; ready > 0; ready--) {
			for (i = 0; i < worker_count; i++) {

				/* Check for each worker if they were ready */
				if (FD_ISSET(workers[i].sock, &fd_set)) {

					/* Clear them from fd_set, so they are not handled again */
					FD_CLR(workers[i].sock, &fd_set);

					/* read the result into the result array */
					read_result(workers[i], results, completed);

					/* Set the done_time to now, because as far as we know, this is the last time we
					 * have heard from this worker */
					clock_gettime(CLOCK_MONOTONIC, &workers[i].done_time);

					/* based on the balancing algorithm, assign a new task */
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

		/* Print a symbol to the progress bar if the amount of completed tasks is divisible by (taskcount/62)
		 * this gives us at max 62 symbols in the progress bar, now matter how many tasks there are. */
		if (completed_tasks % ((task_count / 62)+1) == 0) {
			printf("\u2588");
			fflush(stdout);
		}
	}

	/* print the end of the progressbar */
	printf("\n");

	/* Sum up the results and return that */
	for (i = 0, total_primes = 0; i < task_count; i++) {
		total_primes += results[i];
	}
	return total_primes;
}

/* read_result reads a result from a worker, and marks the given task as completed in the completed array */
void read_result(struct worker worker, result_t *results, int *completed)
{
	struct result from_worker;
	int i;
	recv(worker.sock, &from_worker, sizeof(struct result), MSG_WAITALL);
	i = from_worker.task_number;
	results[i] = from_worker.result;
	completed[i] = 1;
}

/* assign_task tries to find a task in the task array that is currently not assigned or completed.
 * If the algorithm is weighted least connections, it first looks in its own chunk, and if no free task
 * was found, it looks in all the rest of the task array to help other workers.
 * When the algorithm is none, it only takes tasks from its own chunk. If a task is found, it is sent. */
void assign_task(struct worker worker, struct task *tasks, int *task_offsets, int task_count, int worker_count, int *completed, int *assigned, enum balance_type balance_type)
{
	int i;
	int end_offset;
	int task_offset;
	struct task *task;
	
	task = NULL;
	task_offset = task_offsets[worker.id];

	/* Calculate the end_offset to be the last index to look at. */
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
		
	/* Look for task starting from the worker's own offset */
	for (i = task_offset; i < end_offset && task == NULL; i++) {
		if (!completed[i] && !assigned[i]) {
			task = &tasks[i];
			assigned[i] = 1;
		}
	}

	/* If the algorithm is weighted, and no task is found, look through the rest of the array */
	if (task == NULL && balance_type == BALANCE_WEIGHTED) {
		for (i = 0; i < task_offset && task == NULL; i++) {
			if (!completed[i] && !assigned[i]) {
				task = &tasks[i];
				assigned[i] = 1;
			}
		}
	}

	/* If a task was found, send it to the worker */
	if (task != NULL) {
		send(worker.sock, task, sizeof(struct task), 0);
	}
}

/* assign_round_robin_task has a similar function as assign_task, but is made specificly for round robin.
 * It finds tasks for a given worker by starting at the index equal to the worker id. Then it skips worker_count
 * and tries again. */
void assign_round_robin_task(struct worker worker, struct task *tasks, int task_count, int worker_count, int *completed, int *assigned)
{
	int i;
	struct task *task;

	task = NULL;

	/* Find a task with index on the form (index = worker.id + n * worker_count) where n is a positive integer */
	for (i = worker.id; i < task_count && task == NULL; i += worker_count) {
		if (!assigned[i] && !completed[i]) {
			task = &tasks[i];
			assigned[i] = 1;
		}
	}

	/* If a task was found, send it to the worker */
	if (task != NULL) {
		send(worker.sock, task, sizeof(struct task), 0);
	}
}


/* print_delimiter prints a line that is to be part of a table.
 * The position argument specifies if the line is at the top (0),
 * in the middle (1) or in the buttom (2) of a table. */
void print_delimiter(int position){
	int i;

	/* Print a total of 72 symbols */
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

/* print_header prints general information about the run of the program */
void print_header(int number_of_workers, int completed_tasks, double total_time, enum balance_type algo){
	print_delimiter(0);
	printf("\u2502 %-15s \u2502 %-15s \u2502 %-15s \u2502 %-15s \u2502\n", "Workers", "Total tasks", "Total runtime", "Algorithm");
	print_delimiter(1);
	printf("\u2502 %15d \u2502 %15d \u2502 %11.3f sec \u2502 %-15s \u2502\n", number_of_workers, completed_tasks, total_time, algorithm_names[algo]);
	print_delimiter(2);
}

/* print_worker_result prints information about a single worker */
void print_worker_result(struct worker worker, int work_number){
	struct timespec now;
	double time_wasted;
	print_delimiter(1);
	
	clock_gettime(CLOCK_MONOTONIC, &now);
	time_wasted = timespec_to_double(now) - timespec_to_double(worker.done_time);
	printf("\u2502 %15d \u2502 %15d \u2502 %15d \u2502 %11.3f sec \u2502\n", work_number, worker.real_weight, worker.completed_tasks, time_wasted);
}

/* print_results prints all the usefull information about the program, such as worker information */
void print_results(struct worker *workers, int number_of_workers, int number_of_primes, int primes_from, int primes_to, double total_time, enum balance_type algo) {
	int i;
	int completed_tasks;

	/* sum up the total amount of completed tasks */
	for (i = 0, completed_tasks = 0; i < number_of_workers; i++) {
		completed_tasks += workers[i].completed_tasks;
	}
	
	/* General information Header */
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
