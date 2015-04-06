#include <regex.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <errno.h>
#include "../lib/utils.h"

int realtime, verbose;

/*
Split a line at some length (index). 
e.g. split every 10th character.
The split-on character is not dropped.
*/

int main(int argc, char ** argv, char ** environ) 
{

	// http://stackoverflow.com/questions/12252103/read-line-from-stdin-blocking
	//  char * line = NULL fixed this..
	char * line = NULL;
	int i, n, len;
	size_t maxline = 1200;
	long index = 10;
	char *endptr, *str;

	// if you see garbage in output, increase this malloc
	char ** dst = (char ** )malloc(maxline*20 * sizeof(char));

	if (argc > 1) {
		str = argv[1];
		index = strtol(str, &endptr, 10);
	
		if ((errno == ERANGE && (index == LONG_MAX || index == LONG_MIN))
			|| (errno != 0 && index == 0)) {
			perror("strtol");
			exit(EXIT_FAILURE);
		}
		if (endptr == str) {
			fprintf(stderr, "No digits were found\n");
			exit(EXIT_FAILURE);
		}
	} else {
		fprintf(stderr, "Usage: Reads from stdin only.\nspliti <length> < line\necho $LINE | spliti 80\nSee fmt(1)\n");
		exit(EXIT_FAILURE);
	}

	// fprintf(stderr, "index = %ld\n", index);

	// split the lines on an index (length)
	while ((len = getline(&line, &maxline, stdin)) > 0) {
		
		//printf("line (%d)\n", len);
		len = chomp(line, len);
		//printf("line (%d) = \"%s\"\n", len, line);

		n = spliti(dst, line, len, (int)index);

		//printf("getline:  len %d: %s, %s %d\n", len, argv[1], line, n);

		for (i = 0; i < n; i++) {
			//printf("%d: %s\n", i, dst[i]);
			printf("%s\n", dst[i]);
		}

	}
	free(dst);
	return 0;

}

