/********************************
 * Web server implementation, designed to sit on top of ping server
 *
 ********************************/

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define BUFFER_SIZE 1024	//Input buffer size in bytes

#define ERROR_MSG "<html><head><title>Error %d</title></head> \
 <body> \
 <h1>Error %d</h1> \
 We were unable to complete your request. \
 </body> \
 </html>"

/**
 *Parses an HTTP request from buffer, returns a path in the same buffer.  Returns 0 on success, -1 on error
 **/
int parse(char *buffer) {
	return -1;
}

int send_error(int *socket_fd, int error_code) {
	char *buf = malloc(strlen(ERROR_MSG) + 10);
	sprintf(buf, ERROR_MSG, error_code, error_code);
	if (write(*socket_fd, buf, strlen(buf)) < 1)
		fprintf(stderr, "Unable to write error message to socket\n");
	close(*socket_fd);
	free(buf);
	return 0;
}

/**
 * Read from the socket and return an appropriate response.  Returns number of bytes sent, or -1 on error.
 */
int service_request(int *socket_fd) {
	char *request_buffer;

	printf("Servicing web request\n");

	request_buffer = malloc(BUFFER_SIZE);
	if (request_buffer == NULL) {
		send_error(socket_fd, 400);
		free(request_buffer);
		return -1;
	}

	if (read(*socket_fd, request_buffer, BUFFER_SIZE) == -1) {
		send_error(socket_fd, 400);
		free(request_buffer);
		return -1;
	}

	if (parse(request_buffer) == -1) {
		send_error(socket_fd, 400);
		free(request_buffer);
		return -1;
	}

	free(request_buffer);

	send_error(socket_fd, 400);
	return -1;
}