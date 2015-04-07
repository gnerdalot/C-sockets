#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

// our libs
#include "udp-cmd-opts.c"
#include "../lib/unp-inet.h"
#include "../lib/utils.h"

/*
 * Example of server using UDP protocol pg. 287, Stevens
 */

extern struct globalArgs_t globalArgs;
extern int verbose, dryrun, realtime;

int main(int argc, char *argv[], char ** environ) {
 
	do_opts (argc, argv, environ);

	int pid = getpid();

	int i;
	int  sockfd;
	int * nclients;
	nclients = calloc(1, sizeof(int));
	*nclients = 0;
	char ** clients = (char **)calloc(100, 22 * sizeof(char));
	struct sockaddr_in serv_addr, cli_addr;
	char * ip;

	// open udp socketo
	if ( ( sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		fprintf(stderr, "Cannot connect: %s\n", strerror( errno ) );
		exit( errno );

	}
	fprintf(stderr, "%s OK server started (pid %d)\n", argv[0], pid);

	// bind our local address so client can send to us
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family       = AF_INET;
	serv_addr.sin_addr.s_addr  = htonl(INADDR_ANY);
	serv_addr.sin_port         = htons(globalArgs.port);
	ip                         = inet_ntoa( serv_addr.sin_addr );

	// reply on the same port
	//cli_addr.sin_port         = htons(globalArgs.port);
	// udp will reply on source port - because it is stateless no establishing

	if (bind(sockfd, (struct sockaddr *) &serv_addr, (socklen_t) sizeof(serv_addr)) < 0) {
		fprintf(stderr, "server: FAIL bind to %s %d (%d)\n", 
			ip, ntohs(serv_addr.sin_port), globalArgs.port);
		exit(2);
	}
	else {
		if (globalArgs.verbose) {
			fprintf(stderr, "server: OK bind to %s %d (%d)\n", 
				ip, ntohs(serv_addr.sin_port), globalArgs.port);
		}
	}
		
	// fprintf(stderr, "start data ingestion\n");
	dg_cmd(sockfd, (struct sockaddr*) &cli_addr, (socklen_t) sizeof(cli_addr), clients, nclients);

	/* not reached */
	for(i = 0; i < 100; i++) 
		free((char*)clients[i]);

	free((void**)clients);
	free(nclients);

	fprintf(stderr, "exiting");
	exit(7);
	
}

