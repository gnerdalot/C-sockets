#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define MAXBUFG = 4096


// http://www.ibm.com/developerworks/aix/library/au-unix-getopt.html
// options
struct globalArgs_t {
	char *serverip;         /* -s --server <ip>   */
	unsigned int port;      /* -p --port <port>   */
	unsigned int udp_packet_size;      /* -u --udpsize<int>   */
	int dryrun;             /* -y --dryrun        */
	int command;            /* -c --command       */
	int verbose;            /* -v --verbose       */
	char **clients;         /* <ip or hostname>   */
	char *message;          /* <text>             */
	char * match;           /* <text>             */
	short int numclients;   /* number of clients  */
} globalArgs;

void usage (char * me, int rc)
{
	printf("%s - simple udp data transfer\n", me);
	printf("Usage: %s [-c <client ip> | -s <server ip>] -p <port> -u <udp size> -m <message> [-v]\n", me);
	printf("Example:\n  server %% udp-s -cp 9001\n  client %% udp-cmd-c -s 0.0.0.0 -p 9001\n\n");
	printf("Example:\n  server %% udp-cmd-s -cp 9001 -m cmd-\n  client %% udp-cmd-c -s 204.27.60.73 -p 9001\n\n");
	exit(rc);
}

int verbose, dryrun, realtime;
char * match;

void do_opts (int argc, char *argv[], char ** environ)
{

	int longIndex = 0;
	int i, opt = 0;
	int maxlen = 4096;

	globalArgs.serverip = "0.0.0.0";
	globalArgs.port = 9000;
	globalArgs.udp_packet_size = 1200;
	globalArgs.match = NULL;
	globalArgs.numclients = 0;
	globalArgs.message = "hello, world.";

	static struct option longOpts[] = {
		{ "serverip",        required_argument, NULL, 's' },
		{ "port",            required_argument, NULL, 'p' },
		{ "udpsize",         required_argument, NULL, 'u' },
		{ "command",         no_argument,       NULL, 'c' },
		{ "verbose",         no_argument,       NULL, 'v' },
		{ "dryrun",          no_argument,       NULL, 'y' },
		{ "match",           required_argument, NULL, 'm' },
		{ "message",         required_argument, NULL, 'M' },
		{  NULL,             0,                 NULL,  0  },
	};

	// see the command line..
	/*
	for (i = 0; i < argc; i++) {
		printf("%s args: %d: %s\n", me, i, argv[i]);
	}
	*/

	// exit if looking for help or no args
	if (argc < 3) usage(argv[0], 1);

	while (( opt = getopt_long(argc, argv, "s:p:u:m:M:cvy", longOpts, &longIndex)) != -1) {
		
		switch (opt) {

			case 'p':
				//printf("choice p %d - %s\n", opt, optarg);
				globalArgs.port = atoi(optarg);
				break;

			case 'u':
				//printf("choice p %d - %s\n", opt, optarg);
				globalArgs.udp_packet_size = atoi(optarg);
				break;

			case 's':
				//printf("choice s %d - %s\n", opt, optarg);
				globalArgs.serverip = optarg;
				break;

			case 'm':
				//printf("choice s %d - %s\n", opt, optarg);
				globalArgs.match = optarg;
				match = optarg;
				break;

			case 'c':
				globalArgs.command = 1;
				break;

			case 'M':
				globalArgs.message = optarg;
				i = strlen(optarg);
				if (i > maxlen) {
					printf("message longer than %d bytes\n", maxlen);
					exit(3);
				}
				break;

			case 'v':
				//printf("choice v %d\n", opt);
				globalArgs.verbose = 1;
				verbose = 1;
				break;

			case 'y':
				//printf("choice y %d\n", opt);
				globalArgs.dryrun = 1;
				dryrun = 1;
				break;

			case 0:
			default :
				break;
		}
	
	/*	
	// messingf option processing up
		if ((opt = getopt_long(argc, argv, "s:p:m:vy", longOpts, &longIndex))) {
			longIndex = optind;
		}
		*/
		
	}

	globalArgs.clients    = argv + optind;
	globalArgs.numclients = argc - optind;

	if (globalArgs.dryrun == 1 || globalArgs.verbose == 1) {

		printf("globalArgs.serverip           = %s\n", globalArgs.serverip);
		printf("globalArgs.port               = %d\n", globalArgs.port);
		printf("globalArgs.udp_packet_size    = %d\n", globalArgs.udp_packet_size);
		printf("globalArgs.message            = %s\n", globalArgs.message);
		printf("globalArgs.verbose            = %d\n", globalArgs.verbose);
		printf("globalArgs.dryrun             = %d\n", globalArgs.dryrun);
		printf("globalArgs.numclients         = %d\n", globalArgs.numclients);
		if (globalArgs.match != NULL) {
		printf("globalArgs.match              = %s\n", globalArgs.match);
		}

		if (globalArgs.dryrun == 1) {
			exit(0);
		}
	}

}
