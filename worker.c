#include <stdio.h>
<<<<<<< HEAD
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include "shared.h"

#define BUFFER_SIZE 100
#define PORT 8080



int main(void) {
	int sock, connect_status;
	ssize_t numRead;
	const weight_t weight = 2;
	char buffer[BUFFER_SIZE];
	struct sockaddr_in local_adress, remote_address;

	/* Initialise local socket */
	local_adress.sin_family = AF_INET;
	local_adress.sin_addr.s_addr = inet_addr("127.0.0.1");
	local_adress.sin_port = PORT;
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
	send(sock, weight, sizeof(weight_t), 0);
	while ( (numRead = recv(sock, buffer, BUFFER_SIZE, 0)) > 0 ){
		printf("%s", buffer);
		
		


		memset(buffer, 0, BUFFER_SIZE);
	}

=======
#include <math.h>

int is_prime(int lower, int upper);
int main(void)
{

>>>>>>> worker-file-is-prime
	return 0;
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