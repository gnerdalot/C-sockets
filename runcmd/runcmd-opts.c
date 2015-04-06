#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

char me[] = "runcmd";

// http://www.ibm.com/developerworks/aix/library/au-unix-getopt.html
// options
struct globalArgs_t {
	char *command;         /* -c --command <some command>   */
	unsigned int timeout;  /* -w --timeout <timeout>        */
	unsigned int dryrun;            /* -y --dryrun                   */
	unsigned int verbose;           /* -v --verbose                  */
	unsigned int realtime;             /* -p --realtime                    */
} globalArgs;

void usage (int rc)
{
	printf("Run a command. use '-r' to get output instead of waiting for EOF\n");
	printf("\n%s -c <command> [-r|--realtime] [-w|--timeout <sec>] [-v|--verbose] [-y|--dryrun]\n", me);
	printf("\nExamples\n");
	printf("  %s -c 'uptime'\n", me);
	printf("  %s -c 'while :; do date; sleep 1; done' # this will never have output\n", me);
	printf("  %s -c 'while :; do date; sleep 1; done' -r\n", me);
	printf("\nnote: timeout not yet implemented.\n");
	exit(rc);
}

void do_opts (int argc, char *argv[], char ** environ)
{

	int longIndex = 0;
	int opt = 0;
	int havecmd = 0;
	char * dst = calloc(1, sizeof(char[1024]));
	char str = ' ';
	char * joinstr = &str;
	globalArgs.verbose = 0;
	globalArgs.dryrun = 0;
	globalArgs.realtime = 0;

	static struct option longOpts[] = {
		{ "command",  required_argument, NULL, 'c' },
		{ "timeout",  required_argument, NULL, 'w' },
		{ "verbose",  no_argument,       NULL, 'v' },
		{ "realtime", no_argument,       NULL, 'r' },
		{ "help",     no_argument,       NULL, 'h' },
		{  NULL,      0,                 NULL,  0  },
	};

	// see the command line..
	
	/*
	int i = 0;
	for (i = 0; i < argc; i++) {
		printf("%s args: %d: %s\n", me, i, argv[i]);
	}
	*/


	while (( opt = getopt_long(argc, argv, "c:w:rhvy", longOpts, &longIndex)) != -1) {

		// optarg will be a pointer
		
		switch (opt) {

			case 'c':
				//printf("arg c %s\n", optarg);
				globalArgs.command = optarg;
				havecmd = 1;
				break;

			case 'w':
				//printf("arg w %s\n", optarg);
				globalArgs.timeout = atoi(optarg);
				break;

			case 'v':
				//printf("arg v\n");
				globalArgs.verbose++;
				break;

			case 'y':
				//printf("arg y\n");
				globalArgs.dryrun = 1;
				break;

			case 'r':
				//printf("arg r\n");
				globalArgs.realtime = 1;
				break;

			case 'h':
				usage(2);
				break;

			case 0:
			default :
				break;
		}
	
	}

	if (havecmd == 0) usage(2);

	if (globalArgs.dryrun == 1 || globalArgs.verbose == 1) {

		printf("globalArgs.command   = %s\n", globalArgs.command);
		printf("globalArgs.timeout   = %d\n", globalArgs.timeout);
		printf("globalArgs.verbose   = %d\n", globalArgs.verbose);
		printf("globalArgs.dryrun    = %d\n", globalArgs.dryrun);
		printf("globalArgs.realtime  = %d\n", globalArgs.realtime);
		join(dst, argv, argc, joinstr, 512);
		printf("argc: %d  argv: %s\n", argc, dst);

	}

}
