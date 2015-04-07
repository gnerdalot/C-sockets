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
 * Example of server using UDP protocol pg. 287
 */

extern struct globalArgs_t globalArgs;
extern int verbose, dryrun, realtime;

int main(int argc, char *argv[], char ** environ) {

	do_opts (argc, argv, environ);

	int                   sockfd;
	struct sockaddr_in    cli_addr, serv_addr;

	//Zero out socket address
	memset(&serv_addr, 0, sizeof(serv_addr));

	//The address is ipv4
	serv_addr.sin_family       = AF_INET;

	// fill in the structure "serv_addr" with the address of the server 
	// that we want to send to
	// ip_v4 adresses is a uint32_t, convert a string representation of the octets 
	// to the appropriate value
	serv_addr.sin_addr.s_addr  = inet_addr(globalArgs.serverip);
	serv_addr.sin_port         = htons(globalArgs.port);

	// open UDP socket (an internet datagram socket)
	if ( (sockfd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
		printf("client: can't open datagram socket to %s:%d\n", 
			globalArgs.serverip, globalArgs.port);
	}
	else {
		if (globalArgs.verbose) {
			printf("client: OK opened datagram socket to %s:%d\n",
				globalArgs.serverip, globalArgs.port);
		}
	}

	// bind any local address for us
	//bzero((char *) &cli_addr, sizeof(cli_addr));  // zero out
	cli_addr.sin_family        = AF_INET;
	cli_addr.sin_addr.s_addr   = htonl(INADDR_ANY);
	cli_addr.sin_port          = htons(0);
	if (bind(sockfd, (struct sockaddr *) &cli_addr, sizeof(cli_addr)) < 0) {
			printf("client: FAIL bind to %s:%d\n", globalArgs.serverip, globalArgs.port);
	}
	else {
		if (globalArgs.verbose) {
			printf("client: OK bind to %s:%d\n", globalArgs.serverip, globalArgs.port);
		}
	}

	// print output
	// now go sit in this command until program exit
	dg_cli(stdin, sockfd, (struct sockaddr *) &serv_addr, (socklen_t) sizeof(serv_addr));

	close(sockfd);
	return 0;
	
}

