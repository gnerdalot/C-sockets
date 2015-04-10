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

/* read datagram form connectionless socket and write it back to the sender.
 * we never return, as we never know when the datagram client is done.
 * server side
 */


void dg_echo(int sockfd, struct sockaddr *pcli_addr, socklen_t maxclilen)
{

	int n;
	socklen_t clilen;
	char mesg[MAXMESG];

	char * logmesg;
	logmesg = calloc(MAXLOGMESG, sizeof(char));

	for ( ; ; ) {

		clilen = maxclilen;

		// empty out
		memset(&mesg, 0, sizeof mesg);

		n = recvfrom(sockfd, mesg, MAXMESG, 0, pcli_addr, &clilen);

		// recv packet
		if (n < 0) {
			sprintf(logmesg, "%s: recvfrom error\n", __func__);
			log2stderr(logmesg);
		}
		else {
			if (verbose) {
				sprintf(logmesg, "START %s: %s:%u recv %lu bytes\n", __func__,
					inet_ntoa(((struct sockaddr_in *)pcli_addr)->sin_addr),
					((struct sockaddr_in *)pcli_addr)->sin_port, strlen(mesg));
				log2stderr(logmesg);
			}
		}

		// send packet
		if (sendto(sockfd, mesg, n, 0, pcli_addr, clilen) != n) {
			sprintf(logmesg, "%s: sendto error\n", __func__);
			log2stderr(logmesg);
		}
		else {
			if (verbose) {
				sprintf(logmesg, "%s: send to %s:%u - %lu bytes\n", __func__,
					inet_ntoa(((struct sockaddr_in *)pcli_addr)->sin_addr),
					((struct sockaddr_in *)pcli_addr)->sin_port, strlen(mesg));
				log2stderr(logmesg);
			}
		}
	}

	free(logmesg);

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
	char * mesg = calloc(MAXMESG, sizeof(char));
	char * nlines = calloc(10, sizeof(char));
	struct runcmd_t cmd;
	cmd.numlines = 0;
	char dst[MAXLINE]; // why is this needed?? else get dg_cmd: sendto error
	char * buff = malloc(MAXMESG * sizeof(char));
	double t_start;
	char * logmesg;
	logmesg = calloc(MAXLOGMESG, sizeof(char));
	realtime = 0;

	for ( ; ; ) {

		clilen = maxclilen;

		// empty out
		memset(mesg, 0, MAXMESG);
		memset(buff, 0, MAXMESG);

		// get data
		n = recvfrom(sockfd, mesg, MAXMESG, 0, pcli_addr, &clilen);

		// get the time
		gettimeofday(&tim, NULL); // time_t tv_sec, suseconds_t tv_usec
		t_start  = tim.tv_sec + tim.tv_usec/1000000.0;

		// recv
		if (n < 0) {
			fprintf(stderr, "%s: recvfrom error\n", __func__);
		}
		else {
			// register client - returns 1 if a new client
			j = registerClient(pcli_addr, clients, nclients);

			if (verbose) {
				sprintf(logmesg, "START %s: %.6lf %s:%u recv %d bytes\n", __func__,
					t_start,
					inet_ntoa(((struct sockaddr_in *)pcli_addr)->sin_addr),
					((struct sockaddr_in *)pcli_addr)->sin_port, (int)strnlen(mesg, MAXMESG) 
				);
				log2stderr(logmesg);
			}
		}

		if (verbose) {
			sprintf(logmesg, "Clients connected: %d\n", j);
			log2stderr(logmesg);
			printClients(clients, nclients);
		}

		cmd.command = mesg;
		if ( ( memcpy(cmd.command, mesg, sizeof(char[MAXMESG]))) == NULL) {
			exit(23);
		}


		// run the command on the server
		runCmd(&cmd);
		n = cmd.numlines;

		// done with command, now send results back
		// command setup and run
		// send back in pieces that are MAXMESG long until it is all sent back.

		if (verbose > 2) fprintf(stderr, "socket %d: lines: %d\n", sockfd, n);

		// set nlines - not like perl... setting a zero-padded character string
		sprintf(nlines, "%08d\n0", n);

		// first thing sent is the line count
		//nlines[9] = 0; // null terminate because

		if (udp_send_mesg(sockfd, pcli_addr, maxclilen, cmd.out[i], n) < n)
			fprintf(stderr, "%s: sendto error:\n%s", __func__, nlines);

		// note end of command
		chomp(cmd.command, MAXLINE);
		sprintf(logmesg, "END \"%s\" %s:%u\n", 
			cmd.command, 
			inet_ntoa(((struct sockaddr_in *)pcli_addr)->sin_addr), 
			((struct sockaddr_in *)pcli_addr)->sin_port
		);
		log2stderr(logmesg);

		// reset output else will get re-used (read: appended)
		for (i = 0; i < n; i++)
			cmd.out[i][0] = 0;

	}
	free(logmesg);

}


/*
 * print out clients
*/
void printClients(char ** clients, int * nclients)
{
	int i;
	char * logmesg;
	logmesg = calloc(MAXLOGMESG, sizeof(char));
	for(i=0; i < *nclients; i++) {
		// if use printf, will get select error on client side
		sprintf(logmesg, "Client %d/%d: %s\n", i + 1, *nclients, clients[i]);
		log2stderr(logmesg);
	}
	free(logmesg);
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
	char * logmesg;
	logmesg = calloc(MAXLOGMESG, sizeof(char));

	port = malloc(sizeof(char[10]));
	dst = malloc(sizeof(char[1])); // init to something..
	client = calloc(22, sizeof(char));
	sprintf(port, "%d", ((struct sockaddr_in *)client_addr)->sin_port);
	if (verbose) sprintf(logmesg, "%s ip:port = %s:%s\n", __func__, ip, port);

	strcat(client, ip);
	strcat(client, ":");
	strcat(client, port);
	// free(ip);
	free(port);

	if (verbose >= 2) {
		sprintf(logmesg, "%s: client %i add?: %s\n", __func__, *nclients, client);
		log2stderr(logmesg);
	}
	// add client if not seen before.
	if ((n= grep(dst, clients, *nclients, client)) > 0) {
		if (verbose >= 2) fprintf(stderr, "%s: client %s already connected\n", __func__, client);
	}
	else {
		clients[*nclients] = malloc(sizeof(char[100]));
		if ( ( memcpy(clients[*nclients], client, sizeof(char[22]))) != NULL) {
			++*nclients;
			if (verbose >= 2){
				sprintf(logmesg, "%s: client %i added: %s\n", __func__, *nclients, client);
				log2stderr(logmesg);
			}
		}
		else {
			sprintf(logmesg, "%s: Error adding %s to client list", __func__, client);
			log2stderr(logmesg);
		}
	}

	// cleanup
	for (i = 0; i < n; i++) free(dst[i]);
	free(dst);
	free(client);
	free(logmesg);

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

	char * logmesg;
	logmesg = calloc(MAXLOGMESG, sizeof(char));

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
				sprintf(logmesg, "%s: sendto error on socket. Error %s  Sent: exit %i, data: %ibytes:  %s\n",
					__func__, strerror(errno), s, n, sendline);
				log2stderr(logmesg);
				exit(90);
			} else {
				if (verbose) {
					sprintf(logmesg, "START: %s: %s:%u recv %lu bytes\n", 
						__func__,
						inet_ntoa(((struct sockaddr_in *)pserv_addr)->sin_addr),
						((struct sockaddr_in *)pserv_addr)->sin_port,
						strlen(sendline)
					);
					log2stderr(logmesg);
				}
			}

			/* now read a message from the socket and write it to our standard output */

			// printf("sockfd %d\n", sockfd);
			while ( (select(maxfd, &fdvar, NULL, NULL, &timeout)) )
			{
				
				if (! (n = recv(sockfd, recvline, sizeof(recvline), 0)))
					die("n returned from select()");

				if (verbose >= 3) {
					sprintf(logmesg, "%s: select %s:%u recv %lu bytes\n", 
						__func__,
						inet_ntoa(((struct sockaddr_in *)pserv_addr)->sin_addr),
						((struct sockaddr_in *)pserv_addr)->sin_port,
						strlen(recvline)
					);
					log2stderr(logmesg);
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

