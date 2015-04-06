#ifndef _UTILS_H
#define _UTILS_H

// like scriptutils 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>

#define SOMELINES 512 
#define MAXLINE  2048 
#define MAXLINES 8192
#define MAXMESG 600

// https://en.wikipedia.org/wiki/External_variable
extern int verbose;
extern int dryrun;
extern int realtime;

typedef struct runcmd_t {
	char *command;         // string representing the command to run
	int rc;                // return code from command
	int numlines;          // no of lines in output
	int numchars;          // no of chars in output
	int verbose;           // verbosity 
	char out[SOMELINES][MAXLINE];  // command output - arrays 
	char outline[MAXLINE];  // command output - arrays 
	int timestart;         // unix time of command start
	int timend;            // unix time of command end
	int timeout;           // program timeout
} runcmd_t;

typedef struct infile_t {
	char * lines[MAXLINES];
	char * filename;
	int numlines;
	int numchars;
} readfile_t;


// begin DECL 
int runCmd(struct runcmd_t *cmd);
int getStream(struct infile_t *infile, FILE * stream);
int getFile(struct infile_t * dst, char * filename);
int chomp(char * array, int size);
int achomp(char ** array, int size);
int join(char * dst, char ** array, int size, char * joinstr, int max);
int grep(char ** dst, char ** array, int size, char * matchstr);
int split(char ** dst, char * array, int size, char * splitstr);

// global to main
// http://www.tenouk.com/ModuleZ.html


// get a stream
int getStream(struct infile_t *infile, FILE * stream)
{

	char buff[MAXLINE];
	int i = 0;
	int n = 0;

	// first zero out
	memset(buff, 0, MAXLINE);

	if (verbose) printf("Function %s  File: %s\n", __func__, infile->filename );
	// read in output and store in array of pointers

	while (fgets(buff, MAXLINE - 1, stream) != NULL) {
		
		if (verbose >= 2) printf("%s line %d read: %s", __func__, i, buff);

		if (realtime) printf("%s", buff);

		// no overflow
		if (i > MAXLINES - 1) break;

		// emulate push: 
		n = sizeof(buff);
			
		infile->lines[i] = calloc(1, sizeof(buff));
		infile->numchars += n; // dont like this - counts chars past \0
		if (verbose >= 2) printf("buffer had %ldchars\n", strlen(buff));

		memcpy(infile->lines[i], buff, sizeof(buff));

		if (verbose >=2) printf("%s line %d %s copied to lines[%d]: %s\n", __func__, i, buff, i, infile->lines[i]);

		i++;

		// subsequent zero out
		memset(buff, 0, MAXLINE);

	}
	if (verbose) printf("%s: read in %d lines\n", __func__, i);

	// C does not track array sizes, so we have to
	infile->numlines = i;

	return ( i > 0 ) ? 0 : 1;
}


// read a file and store in a struct
int getFile(struct infile_t * dst, char * filename)
{
	int res;
	FILE *in = fopen(filename, "r");

	if (verbose) printf("%s: %s\n", __func__, filename);

	// read the stream and return the exit code
	res = getStream(dst, in);
	res += fclose(in);

	return res;

}


	
// run a command. all info is stored in the passed in runcmd_t struct
int runCmd(struct runcmd_t *cmd)
{

	// TODO - add setitimer/alarm

	FILE *in;
	FILE *popen();
	int i = 0;
	//int j = 0;
	int rc = 0;
	struct infile_t output = { .numlines = 0 };

	// init - causes segfault with runcmd
	/*
	for(i = 0; i < SOMELINES; i++) 
		for (j = 0; j < MAXLINES; j++) 
			cmd->out[i][j] = 0;
	*/	

	// c99 introduced this
	if (cmd->verbose > 0) printf("Function: %s  command: %s\n", __func__, cmd->command );

	if (strlen(cmd->command) < 1) {
		fprintf(stderr, "%s passed zero-length command\n", __func__);
		return 1;
	}

	// run the command
	if (!(in = popen(cmd->command, "r"))) {
		printf("Error with \"%s\"\n", cmd->command);
		cmd->rc = 126;
		return 126;
	}

	// read in the stream
	if (verbose) printf("%s Get input stream%s\n", __func__, cmd->command );
	rc = getStream(&output, in);

	// copy the results into our struct
	for (i = 0; i < output.numlines; i++) {
		if (verbose >= 2) printf("line %d: %s\n", i, output.lines[i]);
		strncat(cmd->out[i], output.lines[i], MAXLINE);
		free(output.lines[i]);
	}
	// if (verbose) printf("%s out \"%s\"\n", __func__, cmd->out[0]);

	cmd->numlines = output.numlines;
	cmd->numchars = output.numchars;
	//achomp(cmd->out, output.numlines);

	// store exit code
	cmd->rc = pclose(in);

	// deal with > 255 (osx goes to 32k for exit codes)
	cmd->rc = (cmd->rc > 255) ? 255 : cmd->rc;
	memset(cmd->outline, 0, MAXLINE);

	for(i =0; i < cmd->numlines; i++) {
		if (strlen(cmd->outline) + strlen(cmd->out[i]) < MAXLINE) {
			strcat(cmd->outline, cmd->out[i]);
		}
	}
	chomp(cmd->outline, MAXLINE);

	if (verbose) printf("%s %s \n%s\nexit %d\n", __func__, cmd->command, cmd->outline, cmd->rc);

	return cmd->rc;

}

// split line on index, return array of arrays
int spliti(char ** dst, char * line, int size, int index)
{
	int i = 0;
	int n = 0;
	int chunksize = MAXMESG;
	char * buff;

	// init
	for(i = 0; i < size; i += index ) {
	
		// create a buffer and zero out
		buff = (char *)calloc((size_t) index, index * sizeof(char));

		// assign it to array
		dst[n] = buff;

		// copy over a chunk of text
		chunksize = (i + index <= size) ? index : size - i;
		memcpy(buff, line + i, chunksize);
		/*
		fprintf(stderr, "%s size = %d, i = %d, n = %d, chunksize = %d, buff = %s\n",
			__func__, size, i, n, chunksize, buff);
		*/

		n++;

	}

	return n;
}



// split on char strings. regex later
int split(char ** dst, char * line, int size, char * splitchar)
{
	int i, j = 0;
	int n = 0;
	char * buff;

	// ensure 1 char
	splitchar[1] = 0;

	// initialize first buffer before entering loop
	buff = (char*)malloc(300*sizeof(char));
	dst[n] = buff;
	memset(buff, 0, MAXLINE);

	for (i = 0; i < size && j < MAXLINE; i++) {

		if (line[i] != splitchar[0]) {

			buff[j] = line[i];
			j++;

		} else {

			// repeated split char
			if (j == 0) continue;

			// new buffer
			dst[n] = buff;
			n++;
			j=0;
			buff = (char*)malloc(300*sizeof(char));
			dst[n] = buff;
			memset(buff, 0, MAXLINE);

			continue;
		}
	}

	free(buff);

	// incremented (n) but buffer has no results, drop it...
	// would happen if split char was last chars on a line
	if(j == 0) n--;


	return n;

}
		

// from array of arrays, build a new one of just substring matches. 
// need to know the array length
// FIX: pcre regex
int grep(char ** dst, char ** array, int size, char * matchstr)
{

	int i, n = 0;
	char * ptr = NULL;

	for (i = 0; i < size; i++ ) {
		
		if (array[i] == NULL) 
			return 0;
		ptr = strstr(array[i], matchstr);

		if ( ptr != NULL ) {
			if (verbose >= 2) printf("%s %s has sub %s\n", __func__, array[i], matchstr);
			dst[n] = calloc(1, sizeof(char[1024]));
			// dst[0] <---------  array[3]
			//     ...
			// dst[1] <---------  array[17]
			//memcpy(dst[n], array[i], sizeof(array[i]));
			memcpy(dst[n], array[i], sizeof(char[1024]));
			n++;
		}
	}

	return n;
}



// from array of arrays, join the string. 
// need to know the array length
int join(char * dst, char ** array, int array_len, char * joinstr, int max)
{

	int i = 0;
	dst[0] = 0;

	for (i = 0; i < array_len; i++ ) {

		strncat(dst, (char *) array[i], MAXLINE);

		// on last element - no join char
		if ( i < array_len) {
			strncat(dst, joinstr, 1);
		}
	}
	return i;
}


// perl's chomp - just a string
int chomp(char * array, int size)
{
	int i;
	for(i = size; i > 0; i--) {
		if (strcmp(&(array[i]), "\n") == 0) {
			array[i] = 0;
			return i;
		}
	}
	return 0;
}

// chomp an array of lines
int achomp(char ** array, int size)
{
	int i;
	for( i = 0; i < size; i++) {
		chomp(array[i], strlen(array[i]));
	}
	return i;
}
		
	



#endif  /* _UTILS_H */

