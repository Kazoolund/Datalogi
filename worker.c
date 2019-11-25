#include <stdio.h>
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

	return 0;
}
