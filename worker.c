#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "shared.h"

#define BUFFER_SIZE 100
#define PORT 8080


int main(void) {
	int sock;
	ssize_t numRead;
	char buffer[BUFFER_SIZE];
	struct sockaddr_in local_adress, remote_address;

	/* Initialise local socket */
	local_adress.sin_family = AF_INET;
	local_adress.sin_addr.s_addr = inet_addr("192.168.0.1");
	local_adress.sin_port = PORT;
	socket(AF_INET, SOCK_STREAM, 0); /* Create client socker */

	bind(socket, (struct sockaddr *)&local_adress, sizeof(local_adress));

	if (sock == -1){
		exit(EXIT_FAILURE);
	}

	/* connect to the remote socket */
	remote_address.sin_family = AF_INET;
	remote_address.sin_addr.s_addr = inet_addr("192.168.0.1");
	remote_address.sin_port = 5000;

	connect(sock, (struct sockaddr *)&remote_address, sizeof(remote_address));
	
	return 0;
}
