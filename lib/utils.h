#ifndef _UTILS_H
#define _UTILS_H

// like scriptutils 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>   // needed for Linux 

#define SOMELINES 512 
#define MAXLINE  2048 
#define MAXLINES 8192
#define MAXMESG 600
#define MAXLOGMESG 512

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
	char **out;            // command output - arrays 
	char outline[MAXLINE]; // command output - string
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
void die(const char *msg);
void log2stderr(char *mesg);
int runCmd(struct runcmd_t *cmd);
int getStream(struct infile_t *infile, FILE * stream);
int udp_send_mesg(int sockfd, struct sockaddr *pcli_addr, socklen_t maxclilen, char ** mesg, int n);
int getFile(struct infile_t * dst, char * filename);
int chomp(char * array, int size);
int achomp(char ** array, int size);
int join(char * dst, char ** array, int size, char * joinstr, int max);
int grep(char ** dst, char ** array, int size, char * matchstr);
int split(char ** dst, char * array, int size, char * splitstr);

// global to main
// http://www.tenouk.com/ModuleZ.html


// perl die
void die(const char *msg)
{
	perror(msg);
	exit(EXIT_FAILURE);
}

/*
 * Log with time
 */
void log2stderr(char * mesg)
{
	struct timeval tim;
	double t_epoch;

	// overflow protection
	mesg[MAXLOGMESG - 1] = 0;

	gettimeofday(&tim, NULL); // time_t tv_sec, suseconds_t tv_usec
	t_epoch  = tim.tv_sec + tim.tv_usec/1000000.0;

	fprintf(stderr, "%.4f %s", t_epoch, mesg);

	// reset
	mesg[0] = 0;

}


// get a stream
int getStream(struct infile_t *infile, FILE * stream)
{

	char buff[MAXLINE];
	int i = 0;
	int n = 0;

	// first zero out
	memset(buff, 0, MAXLINE);

	if (verbose >= 2) {
		fprintf(stderr, "Function %s  File: %s\n", __func__, infile->filename );
	}
	// read in output and store in array of pointers

	while (fgets(buff, MAXLINE - 1, stream) != NULL) {
		
		if (verbose >= 2) {
			fprintf(stderr, "%s line %d read: %s", __func__, i, buff);
		}

		if (realtime) printf("%s", buff);

		// no overflow
		if (i > MAXLINES - 1) break;

		// emulate push: 
		n = sizeof(buff);
			
		infile->lines[i] = calloc(1, sizeof(buff));
		infile->numchars += n; // dont like this - counts chars past \0
		if (verbose >= 2) {
			fprintf(stderr, "buffer had %ldchars\n", strlen(buff));
		}

		memcpy(infile->lines[i], buff, sizeof(buff));

		if (verbose >=2) {
			fprintf(stderr, "%s line %d %s copied to lines[%d]: %s\n", __func__, i, buff, i, infile->lines[i]);
		}

		i++;

		// subsequent zero out
		memset(buff, 0, MAXLINE);

	}

	if (verbose >= 2) {
		fprintf(stderr, "%s: read in %d lines\n", __func__, i);
	}

	// C does not track array sizes, so we have to
	infile->numlines = i;

	return ( i > 0 ) ? 0 : 1;
}


// read a file and store in a struct
int getFile(struct infile_t * dst, char * filename)
{
	int res;
	FILE *in = fopen(filename, "r");

	if (verbose >= 2) {
		fprintf(stderr, "%s: %s\n", __func__, filename);
	}

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
	char * logmesg = calloc(MAXLOGMESG, sizeof(char));

	// c99 introduced this
	if (cmd->verbose > 0) {
		fprintf(stderr, "Function: %s  command: %s\n", __func__, cmd->command );
	}

	if (strlen(cmd->command) < 1) {
		sprintf(logmesg, "%s passed zero-length command\n", __func__);
		log2stderr(logmesg);
		return 1;
	}

	// run the command
	if (!(in = popen(cmd->command, "r"))) {
		printf("Error with \"%s\"\n", cmd->command);
		cmd->rc = 126;
		return 126;
	}

	// read in the stream
	if (verbose >= 2) {
		fprintf(stderr, "%s Get input stream%s\n", __func__, cmd->command );
	}
	rc = getStream(&output, in);

	cmd->out = (char **)calloc(MAXLINES, sizeof(char[MAXLINE]));
	// copy the results into our struct
	for (i = 0; i < output.numlines; i++) {
		if (verbose >= 2) {
			fprintf(stderr, "--> line %d: %s", i, output.lines[i]);
		}
		cmd->out[i] = output.lines[i]; 
		// , sizeof(char[MAXLINE]));
		// strncat(cmd->out[i], output.lines[i], MAXLINE);
	}

	cmd->numlines = output.numlines;
	cmd->numchars = output.numchars;
	// achomp(cmd->out, output.numlines); // needed else recv does not delimit well

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
	chomp(cmd->command, MAXLINE);

	if (verbose == 1) {
		sprintf(logmesg, "%s \"%s\" exit %d\n", __func__, cmd->command, cmd->rc);
		log2stderr(logmesg);
	}
	else if (verbose > 1) {
		sprintf(logmesg, "%s \"%s\" exit %d\n%s", __func__, cmd->command, cmd->rc, cmd->outline);
		log2stderr(logmesg);
	}

	if (logmesg != NULL)
		free(logmesg);

	return cmd->rc;

}


// Send a message via udp to one dest
// mesg is array of arrays
int udp_send_mesg(int sockfd, struct sockaddr *pcli_addr, socklen_t maxclilen, char ** mesg, int n)
{
	int i = 0;
	struct timeval tim;
	double t_end;
	size_t lines = 10;
	char * logmesg;
	logmesg = calloc(MAXLOGMESG, sizeof(char));
	char * nlines = calloc(10, sizeof(char));

	// set nlines - not like perl... setting a zero-padded character string
	sprintf(nlines, "%08d\n", n);

	// first thing sent is the line count
	nlines[9] = 0; // null terminate because
	sprintf(logmesg, "%s: send %d lines\n", __func__, n);
	log2stderr(logmesg);

	if (sendto(sockfd, nlines, lines, 0, pcli_addr, maxclilen) < strlen(nlines)) {
		sprintf(logmesg, "%s: sendto error:\n%s", __func__, nlines);
		log2stderr(logmesg);
	}

	// send the lines
	for (i = 0; i < n; i++) {

		strcat(mesg[i], "\n");
		if (sendto(sockfd, mesg[i], strlen(mesg[i]), 0, pcli_addr, maxclilen) < strlen(mesg[i])) {
			sprintf(logmesg, "%s: sendto error:\n%s", __func__, mesg[i]);
		}
		else {
			if (verbose) {
				gettimeofday(&tim, NULL); // time_t tv_sec, suseconds_t tv_usec
				t_end  = tim.tv_sec + tim.tv_usec/1000000.0;
				sprintf(logmesg, "%s: %.6lf: send to %s:%u %lu bytes\n",
					__func__,
					t_end,
					inet_ntoa(((struct sockaddr_in *)pcli_addr)->sin_addr),
					((struct sockaddr_in *)pcli_addr)->sin_port,
					strlen(mesg[i])
				);
				log2stderr(logmesg);
			}
		}
	}
	// return lines sent
	return i;

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
	buff = (char*)calloc(300, sizeof(char));
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
			buff = (char*)calloc(300, sizeof(char));
			dst[n] = buff;
			memset(buff, 0, MAXLINE);

			continue;
		}
	}

	if (buff != NULL)
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
			if (verbose >= 2) {
				fprintf(stderr, "%s %s has sub %s\n", __func__, array[i], matchstr);
			}
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
	for(i = 0; i < size; i++) {
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

