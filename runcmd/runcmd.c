#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include "../lib/utils.h"
#include "./runcmd-opts.c"

// https://en.wikipedia.org/wiki/External_variable
extern struct globalArgs_t globalArgs_t;

int verbose, dryrun, realtime;

int main(int argc, char *argv[], char ** environ) {
 
 	// initialize
	if (argc == 1) {
		usage(2);
	}
	
	struct runcmd_t cmd = { 
		.numlines = 0,
		.rc = 0,
		//.out[511][2047] = 0,
		//.out[0][0] = 0,
		.verbose = 0, 
		.command = calloc(1, sizeof(char[1024]))
	}; 

	dryrun   = 0;
	realtime = 0;

	int i;
	char * dst = calloc(1, sizeof(char[1024]));
	char str = ' ';
	char * joinstr = &str;

	// get and set options
	do_opts (argc, argv, environ);

	// join
	// int join(char * dst, char ** array, int array_len, char * joinstr, int max)
	join(dst, argv, argc, joinstr, 512);

	// set post args
	memcpy(cmd.command, globalArgs.command, strnlen(globalArgs.command, 512));
	cmd.verbose  = globalArgs.verbose;
	dryrun       = globalArgs.dryrun;
	realtime     = globalArgs.realtime;

	globalArgs.command = NULL;

	if (cmd.verbose) {
		printf("%d args = %s\n", argc, dst);
		printf("cmd.command = %s\n", cmd.command);
		printf("verbose     = %d\n", cmd.verbose);
		printf("dryrun      = %d\n", dryrun);
		printf("realtime    = %d\n", realtime);
	}

	if (globalArgs.dryrun) exit(0);

	// run the command
	runCmd(&cmd);

	// don't print twice
	if (! globalArgs.realtime) {
		for (i = 0; i < cmd.numlines; i++ )
			printf("%s", cmd.out[i]);
	}

	exit(cmd.rc);

}

