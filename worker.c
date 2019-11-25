#include <stdio.h>

#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include "shared.h"
#include "prime.h"
#include <math.h>
#include <inttypes.h>

#define ARGV_WEIGHT 1

#define REMOTE_IP "127.0.0.1"


struct result do_task(struct task new_task, weight_t weight);
int connect_socket_n(int sock, struct sockaddr_in remote_address );
int send_result(int sock, struct result send_message);
struct task wait_for_next_task(int sock);

void socket_run_time(int sock, weight_t weight);


int main(int argc, char *argv[]) {
	weight_t worker_weight = 0;
	int sock;

	if (argc != 2){
		printf("Remember to input worker weight, exiting.... \n");
		exit(EXIT_FAILURE);
	}
	
	sscanf(argv[ARGV_WEIGHT], "%" SCNu8, &worker_weight);

	if (worker_weight < 0){
		printf("no input given for weight exiting....\n");
		exit(EXIT_FAILURE);
	}

	/* initialse socket */
	sock = socket(AF_INET, SOCK_STREAM, 0); /* Create client socket */


	/* check that socket was successfully initialized */
	if (sock < 0){
		printf("Error creating local socket exiting....\n");
		exit(EXIT_FAILURE);
	}
	
	/* everything has progressed as expected so far, run the main program */
	socket_run_time(sock, worker_weight);


}

void socket_run_time(int sock, weight_t weight){
	struct sockaddr_in remote_address;
	struct result results;
	struct task buffer;
	ssize_t numRead;
	int connection_status;
	int initial_send;
	int send_result_n;

	remote_address.sin_family = AF_INET;
	remote_address.sin_addr.s_addr = inet_addr(REMOTE_IP);
	remote_address.sin_port = htons(LOADBALANCER_PORT);
	/* Try to connect to master */
	connection_status = connect_socket_n(sock, remote_address);
	/* send weight to master */
	initial_send = send(sock, &weight, sizeof(weight_t), 0);

	if (initial_send < 0){
		printf("Initial weight sent to master failed, exiting.... \n");
		exit(EXIT_FAILURE);
	}

	
	while ( (numRead = recv(sock, &buffer, sizeof(struct task), MSG_WAITALL)) > 0 ){
		
		printf("from %d, to %d, task num %d\n", buffer.from, buffer.to, buffer.task_number);

		results = do_task(buffer, weight);
		send_result(sock, results);

	}
	

}



struct result do_task(struct task new_task, weight_t weight) {
	struct result new_result;
	new_result.result = weighted_prime_worker(weight, new_task);
	new_result.task_number = new_task.task_number;
	return new_result;
}

int connect_socket_n(int sock, struct sockaddr_in remote_address ) {
	int connect_status;
	connect_status = connect(sock, (struct sockaddr *)&remote_address, sizeof(remote_address));
	return connect_status;
}

int send_result(int sock, struct result send_message){
	int send_status;
	send_status = send(sock, &send_message, sizeof(struct result), 0);
	if (send_status < 0){
		printf("results failed, running again \n");
		return send_result(sock, send_message);
	}
	return send_status;
}

struct task wait_for_next_task(int sock){
	struct task buffer;
	ssize_t numRead;
	while ( (numRead = recv(sock, &buffer, sizeof(struct task), MSG_WAITALL)) > 0 ){
		if (buffer.task_number != 0 && buffer.from > 0 && buffer.to > 0){
			break;
		}

	}
	return buffer;
}
