#ifndef _UNP_INET_H
#define _UNP_INET_H

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/time.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>

// our libs
#include "utils.h"

#define MAXLINE 2048 
// #define MAXMESG 1200 // far below standard MTU of 1500

extern int verbose, dryrun, realtime;


void dg_echo(int sockfd, struct sockaddr *pcli_addr, socklen_t maxclilen);
void dg_cmd(int sockfd, struct sockaddr *pcli_addr, socklen_t maxclilen, char ** clients, int * nclients);
int registerClient(struct sockaddr *client_addr, char ** clients, int * nclients);
void printClients(char ** clients, int * nclients);
void dg_cli(FILE *fp, int sockfd, struct sockaddr *pserv_addr, socklen_t servlen);
void die(const char *msg);

/* read datagram form connectionless socket and write it back to the sender.
 * we never return, as we never know when the datagram client is done.
 * server side
 */

void die(const char *msg)
{
	perror(msg);
	exit(EXIT_FAILURE);
}

void dg_echo(int sockfd, struct sockaddr *pcli_addr, socklen_t maxclilen)
{

	int n;
	socklen_t clilen;
	char mesg[MAXMESG];

	for ( ; ; ) {
		clilen = maxclilen;

		// empty out
		memset(&mesg, 0, sizeof mesg);

		n = recvfrom(sockfd, mesg, MAXMESG, 0, pcli_addr, &clilen);

		if (n < 0) {
			fprintf(stderr, "%s: recvfrom error\n", __func__);
		}
		else {
			if (verbose >= 1) {
				fprintf(stderr, "%s: %s:%u recv %lu bytes\n", __func__, 
					inet_ntoa(((struct sockaddr_in *)pcli_addr)->sin_addr), 
					((struct sockaddr_in *)pcli_addr)->sin_port, strlen(mesg));
			}
		}

		// send packet
		if (sendto(sockfd, mesg, n, 0, pcli_addr, clilen) != n) {
			if (verbose) fprintf(stderr, "%s: sendto error\n", __func__);
		}
		else {
			if (verbose == 1) {
				fprintf(stderr, "%s: send to %s:%u - %lu bytes\n", __func__, 
					inet_ntoa(((struct sockaddr_in *)pcli_addr)->sin_addr), 
					((struct sockaddr_in *)pcli_addr)->sin_port, strlen(mesg));
			}
		}
	}
}


/* read datagram from connectionless socket, run that command and write it 
 * back to the sender we never return, as we never know when the datagram client is done.
 * server side
 */

void dg_cmd(int sockfd, struct sockaddr *pcli_addr, socklen_t maxclilen, char ** clients, int * nclients)
{

	int i, j, n;
	struct timeval tim;
	socklen_t clilen;
	char mesg[MAXMESG];
	char * nlines = malloc(10 * sizeof(char));
	struct runcmd_t cmd;
	cmd.numlines = 0;
	char dst[MAXLINE]; // why is this needed?? else get dg_cmd: sendto error
	char * buff = malloc(MAXMESG * sizeof(char));
	double t_start;
	double t_end;

	for ( ; ; ) {
		clilen = maxclilen;

		// empty out
		memset(&mesg, 0, MAXMESG);
		memset(buff, 0, MAXMESG);

		n = recvfrom(sockfd, mesg, MAXMESG, 0, pcli_addr, &clilen);

		// get the time
		gettimeofday(&tim, NULL); // time_t tv_sec, suseconds_t tv_usec
		t_start  = tim.tv_sec + tim.tv_usec/1000000.0;

		if (n < 0) {
			fprintf(stderr, "%s: recvfrom error\n", __func__);
		}
		else {
			// register client - returns 1 if a new client
			j = registerClient(pcli_addr, clients, nclients);

			if (verbose) {
				fprintf(stderr, "%s: %.6lf %s:%u recv %lu bytes\n", __func__, 
					t_start,
					inet_ntoa(((struct sockaddr_in *)pcli_addr)->sin_addr), 
					((struct sockaddr_in *)pcli_addr)->sin_port, strlen(mesg));
			}
		}

		if (verbose) {
			fprintf(stderr, "Clients connected: %d\n", j);
			printClients(clients, nclients);
		}

		// command setup and run
		cmd.command = mesg;
		cmd.numlines = 0;
		cmd.numchars = 0;
		realtime = 0;

		// run the command on the server
		runCmd(&cmd);

		// match as needed

		// dont know until command is done how much big matchlines
		// should be, so size it here
		//matchlines = malloc(sizeof(cmd.out));

		if (match != NULL) {
			//n = grep(matchlines, cmd.out, cmd.numlines, match);
			// for(i = 0; i < n; i++) printf("%d %s", i, matchlines[i]);
		}
		else {
			// printf("no matching %s\n", match);
			//matchlines = cmd.out;	
			n = cmd.numlines;
		}

		n = cmd.numlines;

		// done with command, now send results back
		// send back in pieces that are MAXMESG long until it is all sent back.

		fprintf(stderr, "socket %d: lines: %d\n", sockfd, n);

		// set nlines - not like perl...
		sprintf(nlines, "%8d\n", n);

		// first thing sent is the line count
		nlines[9] = 0;
		if (sendto(sockfd, nlines, 10, 0, pcli_addr, clilen) < 10) 
			fprintf(stderr, "%s: sendto error:\n%s", __func__, nlines);

		// now send the lines
		for (i = 0; i < n; i++) {

			if (sendto(sockfd, cmd.out[i], strlen(cmd.out[i]), 0, pcli_addr, clilen) 
				< strlen(cmd.out[i])) {
				fprintf(stderr, "%s: sendto error:\n%s", __func__, cmd.out[i]);
			}
			else {
				if (verbose == 1) {
					gettimeofday(&tim, NULL); // time_t tv_sec, suseconds_t tv_usec
					t_end  = tim.tv_sec + tim.tv_usec/1000000.0;
					fprintf(stderr, "%s: %.6lf send to %s:%u %lu bytes\n", __func__, 
						t_end,
						inet_ntoa(((struct sockaddr_in *)pcli_addr)->sin_addr), 
						((struct sockaddr_in *)pcli_addr)->sin_port, strlen(cmd.out[i]));
					fprintf(stderr, "Elapsed: %.6lf\n----------\n", t_end - t_start);
				}
			}
		}

		// reset output else will get re-used.
		for (i = 0; i < n; i++)
			cmd.out[i][0] = 0; 

	}

}


/*
 * print out clients
*/
void printClients(char ** clients, int * nclients)
{
	int i;
	for(i=0; i < *nclients; i++) {
		// if use printf, will get select error on client side
		fprintf(stderr, "Client %d/%d: %s\n", i + 1, *nclients, clients[i]);
	}
}


/* 
 * register a new client thus assumes ip:port = one client
*/

int registerClient(struct sockaddr *client_addr, char ** clients, int * nclients)
{

	int i, n = 0;
	char ** dst; // for grep results

	// clients
	char * client; // ip:port pair stored in client string
	char * ip  = inet_ntoa( ((struct sockaddr_in *)client_addr)->sin_addr );
	char * port;

	port = malloc(sizeof(char[10]));
	dst = malloc(sizeof(char[1])); // init to something..
	client = calloc(1, sizeof(char[22]));
	sprintf(port, "%d", ((struct sockaddr_in *)client_addr)->sin_port);
	if (verbose) fprintf(stderr, "%s ip:port = %s:%s\n", __func__, ip, port);

	strcat(client, ip);
	strcat(client, ":");
	strcat(client, port);
	// free(ip);
	free(port);

	if (verbose) fprintf(stderr, "%s: client %i add?: %s\n", __func__, *nclients, client);
	// add client if not seen before.
	if ((n= grep(dst, clients, *nclients, client)) > 0) {
		if (verbose) fprintf(stderr, "%s: client %s already connected\n", __func__, client);
	}
	else {
		clients[*nclients] = malloc(sizeof(char[100]));
		if ( ( memcpy(clients[*nclients], client, sizeof(char[22]))) != NULL) {
			if (verbose) fprintf(stderr, "%s: client %i added: %s\n", __func__, *nclients, client);
			++*nclients;
		}
		else {
			fprintf(stderr, "%s: Error adding %s to client list", __func__, client);
		}
	}

	for (i = 0; i < n; i++) free(dst[i]);
	free(dst);
	free(client);

	return *nclients;

}



/* read the contents of the file *fp, write each line to the datagram socket, 
 * then read a line back from the datagram socket and write it to the standard output.
 *
 * Return to caller when an EOF is encountered on the input file client side
 */


void dg_cli(FILE *fp, int sockfd, struct sockaddr *pserv_addr, socklen_t servlen)
{

	int n, s, line, nlines;
	char prompt[] = ">> ";
	char sendline[MAXLINE], recvline[MAXLINE + 1];

	static struct timeval timeout;
	timeout.tv_sec = 2;
	timeout.tv_usec = 500;

	fd_set fdvar;
	int maxfd = sockfd + 1;

	FD_ZERO(&fdvar);  // initialize the set - all bits off */
	FD_SET(sockfd, &fdvar); // turn on bit for sockfd

	while (1) {

		line = 0;
		nlines = 0;
		fprintf(stderr, "%s", prompt);

		// take the input from the prompt
		//while(fgets(sendline, MAXLINE, fp) != NULL) {
		if(fgets(sendline, MAXLINE, fp) != NULL) {

			n = strlen(sendline);

			// send to remote address
			if ((s = sendto(sockfd, sendline, sizeof(sendline), 0, pserv_addr, (socklen_t) servlen)) < 0) {
				fprintf(stderr, "%s: sendto error on socket. Error %s  Sent: exit %i, data: %ibytes:  %s\n", 
					__func__, strerror(errno), s, n, sendline);
				exit(90);
			} else {
				if (verbose == 1) {
					fprintf(stderr, "%s: %s:%u recv %lu bytes\n", __func__, 
						inet_ntoa(((struct sockaddr_in *)pserv_addr)->sin_addr), 
						((struct sockaddr_in *)pserv_addr)->sin_port, strlen(sendline));
				}
			}

			/* now read a message from the socket and write it to our standard output */

			// printf("sockfd %d\n", sockfd);
			while ( (select(maxfd, &fdvar, NULL, NULL, &timeout)) )
			{
				
				if (! (n = recv(sockfd, recvline, sizeof(recvline), 0)))
					die("n returned from select()");

				if (verbose == 1) {
					fprintf(stderr, "%s: select %s:%u recv %lu bytes\n", __func__, 
						inet_ntoa(((struct sockaddr_in *)pserv_addr)->sin_addr), 
						((struct sockaddr_in *)pserv_addr)->sin_port, strlen(recvline));
				}

				recvline[n] = 0;
				//fprintf(stdout, "%d/%d %s", line, nlines, recvline);
				if (line > 0) fprintf(stdout, "%s", recvline);

				// the first line is the line count
				if (line == 0) nlines = atoi(recvline);

				if (line == nlines) {
					fprintf(stderr, "------ %d of %d lines -------\n", line, nlines);
					break;
				}

				line++;

			}

		} // end while fgets

	} // end for ;;

}

// # vim: set nu ai ts=4

#endif /* end _UNP_INET_H */

