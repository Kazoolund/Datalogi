#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include "shared.h"
#include <math.h>

#define ARGV_WEIGHT 1

#define LOCAL_IP "127.0.0.1"
#define LOCAL_PORT 8080
#define REMOTE_IP "127.0.0.1"
#define REMOTE_PORT 5000


struct result do_task(struct task new_task);
int connect_socket_n(int sock, struct sockaddr_in remote_address );
int send_result(int sock, struct result send_message);
struct task wait_for_next_task(int sock);

void socket_run_time(int sock, weight_t weight);


int main(int argc, char *argv[]) {
	weight_t worker_weight = 0;
	int sock;
	struct sockaddr_in local_address;

	if (worker_weight < 0){
		printf("no input given for weight exiting....\n");
		exit(EXIT_FAILURE);
	}

	/* initialse socket */
	local_address.sin_family = AF_INET;
	local_address.sin_addr.s_addr = inet_addr(LOCAL_IP);
	local_address.sin_port = LOCAL_PORT;
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
	remote_address.sin_port = htons(REMOTE_PORT);
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

		results = do_task(buffer);
		send_result(sock, results);

	}
	

}


int fmain(int argc, char *argv[]) {
	int sock, connect_status;
	ssize_t numRead;
	weight_t weight = 2;
	struct result resultn;
	resultn.result = 5;
	resultn.task_number = 1;
	struct task buffer;

	struct sockaddr_in local_adress, remote_address;

	/* Initialise local socket */
	local_adress.sin_family = AF_INET;
	local_adress.sin_addr.s_addr = inet_addr("127.0.0.1");
	local_adress.sin_port = LOCAL_PORT;
	sock = socket(AF_INET, SOCK_STREAM, 0); /* Create client socket */

	if (sock == -1){
		exit(EXIT_FAILURE);
	}

	/* connect to the remote socket */
	remote_address.sin_family = AF_INET;
	remote_address.sin_addr.s_addr = inet_addr("127.0.0.1");
	remote_address.sin_port = htons(5000);

	connect_status = connect(sock, (struct sockaddr *)&remote_address, sizeof(remote_address));

	/* send weight to master */
	send(sock, &weight, sizeof(weight_t), 0);

	send(sock, &resultn, sizeof(struct result), 0);
	while ( (numRead = recv(sock, &buffer, sizeof(struct task), MSG_WAITALL)) > 0 ){
		
		printf("from %d, to %d, task num %d\n", buffer.from, buffer.to, buffer.task_number);


	}
}



struct result do_task(struct task new_task) {
	struct result new_result;
	/* Implement  */
	return new_result ;
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




/*
 * Function int is_prime(int lower, int upper) (FOR DEMO PURPOSES)
 * Takes two inputs: int lower and int upper.
 * int lower represents the lower limits of the numbers to be checked
 * int upper represents the upper limits of the numbers to be checked
 * 
 * When all numbers from lower to upper has been checked, an int value of the 
 * counted primes is returned.
 */
int is_prime(int lower, int upper) {
	int count = 0;
	int number_to_check;

	for (number_to_check = lower; number_to_check <= upper; number_to_check++)
	{	
		int division;
		int isprime = 1;
		for (division = 1.0; division < number_to_check; division++)
		{	
			double divided = (number_to_check / (double)division); /* One of the integars has to be considered a double to return a double */
			double fraction = fmod(divided, 1);

			if (division != 1 && division != number_to_check && fraction == 0.00000000)
				isprime = 0;
		}

		if (isprime)
			count++;
	}
	
	return count;
}