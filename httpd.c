/********************************
 * Web server implementation, designed to sit on top of ping server
 *
 ********************************/

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>

#define BUFFER_SIZE 1024	//Input buffer size in bytes

#define DEBUG 2

#define ERROR_MSG "<html><head><title>Error %d</title></head> \
 <body> \
 <h1>Error %d</h1> \
 We were unable to complete your request. \
 </body> \
 </html>"

#define OK_MSG "HTTP/1.1 200 OK \r\nContent-Type: text/html \r\n\r\n"

static char *serverPath;

/**
 * Parses an HTTP request from buffer, returns a path in the same buffer.
 * Returns 0 on success, -1 on error, -2 on illegal request
 **/
int parse(char *buffer) {
	int version_major;
	int version_minor;
	char *temp_buf;

	if (sscanf(buffer, "GET %s HTTP/%d.%d \r\n\r\n", buffer, &version_major, &version_minor) != 3) {
		fprintf(stderr, "Failed to scan client request: %s\n", buffer);
		return -1;
	}
	if (DEBUG)
		printf("Received request for %s (HTTP %d.%d)\n", buffer, version_major, version_minor);
	
	if (strstr(buffer,"../") != NULL) {
		fprintf(stderr, "Illegal request: %s\n", buffer);
		return -2;
	}

	if (strlen(buffer) + strlen(serverPath) + 1 > BUFFER_SIZE) {
		fprintf(stderr, "Request too long for buffer: %s%s\n", serverPath,buffer);
		return -1;	
	}

	temp_buf = malloc(strlen(buffer) + 1);
	strcpy(temp_buf, buffer);
	sprintf(buffer,"%s%s",serverPath,temp_buf);
	free(temp_buf);

	return 0;
}

/**
 * Sends an error to the client
 **/
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
 * Parses an proposed server root
 **/
int parse_root(char *path) {
	serverPath = path;
	return 0;
}

/**
 * Read from the socket and return an appropriate response.  Returns number of bytes sent, or -1 on error.
 */
int service_request(int *socket_fd) {
	char *request_buffer;
	int result;
	int content_fd;
	int size;

	request_buffer = malloc(BUFFER_SIZE);
	if (request_buffer == NULL) {
		send_error(socket_fd, 500);
		free(request_buffer);
		return -1;
	}

	if (read(*socket_fd, request_buffer, BUFFER_SIZE) == -1) {
		send_error(socket_fd, 400);
		free(request_buffer);
		return -1;
	}

	result = parse(request_buffer);
	if (result == -1) {
		send_error(socket_fd, 400);
		free(request_buffer);
		return -1;
	}
	if (result == -2) {
		send_error(socket_fd, 401);
		free(request_buffer);
		return -1;
	}

	//Open the requested file
	if (DEBUG > 1)
		printf("Opening %s\n", request_buffer);
	content_fd = open(request_buffer, O_RDONLY);

	if (content_fd < 0) {
		send_error(socket_fd, 404);
		free(request_buffer);
		return -1;	
	}

	//Send an OK
	if (write(*socket_fd, OK_MSG, strlen(OK_MSG)) < 1) {
		fprintf(stderr, "Unable to write OK message to socket\n");
		goto cleanup;
	}

	//Send the file
	while ((size = read(content_fd, request_buffer, BUFFER_SIZE)) > 0) {
		printf("Read %d bytes\n",size);
		result = write(*socket_fd, request_buffer, size);
		printf("Wrote %d bytes\n", result);
		if (result < size) {
			fprintf(stderr, "Error writing file to socket\n");
			goto cleanup;
		}
	}

	//Clean up
	cleanup:
	close(content_fd);
	free(request_buffer);
	shutdown(*socket_fd, 2);
	close(*socket_fd);
	return 0;
}