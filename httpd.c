/********************************
 * Web server implementation, designed to sit on top of ping server
 *
 ********************************/

#include <stddef.h>
#include <stdlib.h>

#define BUFFER_SIZE 1024;	//Input buffer size in bytes

/**
 * Read from the socket and return an appropriate response.  Returns number of bytes sent, or -1 on error.
 */
int service_request(int *socket_fd) {
	char *request_buffer;

	request_buffer = malloc(BUFFER_SIZE);
	if (request_buffer == NULL)
		return -1

}