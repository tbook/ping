/************************************************
 *
 *  Header file for httpd
 *
 ************************************************/

int service_request(int *socket_fd);
int parse_root(char *path);

static char *serverPath;
