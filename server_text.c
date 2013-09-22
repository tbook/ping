#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include "httpd.h"
#include <signal.h>
#include <netdb.h>
#include <ifaddrs.h>

#define SIZE_WIDTH 2
#define TIME_WIDTH 8
#define BUF_LEN (2<<16) // though the max for unsigned short is 2<<16-1, 2<<16 is good enough.

/**************************************************/
/* a few simple linked list functions             */
/**************************************************/


/* A linked list node data structure to maintain application
   information related to a connected socket */
struct node {
  int socket;
  struct sockaddr_in client_addr;
  int pending_data; /* flag to indicate whether there is more data to send */
  /* you will need to introduce some variables here to record
     all the information regarding this socket.
     e.g. what data needs to be sent next */
  struct node *next;
  char* buffer;
  int n_bytes; // number of bytes sent or received. pending_data is used for marking which case it is. 
  int msg_size;
};

struct node* root;

/* remove the data structure associated with a connected socket
   used when tearing down the connection */
void dump(struct node *head, int socket) {
  struct node *current, *temp;

  current = head;

  while (current->next) {
    if (current->next->socket == socket) {
      /* remove */
      temp = current->next;
      current->next = temp->next;
      free(temp->buffer); //free the buffer allocated for this node.
      free(temp); /* don't forget to free memory */
      return;
    } else {
      current = current->next;
    }
  }
}

/* create the data structure associated with a connected socket */
void add(struct node *head, int socket, struct sockaddr_in addr) {
  struct node *new_node;

  new_node = (struct node *)malloc(sizeof(struct node));
  new_node->socket = socket;
  new_node->client_addr = addr;
  new_node->pending_data = 0;
  new_node->next = head->next;
  new_node->buffer = (char*)malloc(sizeof(char)*BUF_LEN);
  new_node->n_bytes = 0;
  new_node->msg_size = 0;
  head->next = new_node;
}

void free_resource()
{
  struct node* tmp;
  while(root->next){
    tmp = root->next;
    root->next = tmp->next;
    close(tmp->socket);
    free(tmp->buffer);
    free(tmp);
  }
}

/*This signal hander will call the resource freer*/
void exit_handler(int sig)
{
  free_resource();
  printf("force close.\n");
  exit(1);
}

/*so that we'll know on which clear server is this server running*/
void get_local_ip()
{
  struct ifaddrs *ifaddr, *ifa;
  int family, s;
  char host[NI_MAXHOST];

  if (getifaddrs(&ifaddr) == -1) {
    perror("getifaddrs");
    exit(EXIT_FAILURE);
  }

  for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
    family = ifa->ifa_addr->sa_family;

    if (family == AF_INET) {
      s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in),
                                               host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
      if (s != 0) {
        printf("getnameinfo() failed: %s\n", gai_strerror(s));
        exit(EXIT_FAILURE);
      }

      if(!strcmp(ifa->ifa_name, "bond0"))
        printf("local server is %s.\n", host);
    }
  }
}


/*****************************************/
/* main program                          */
/*****************************************/

/* simple server, takes one parameter, the server port number */
int main(int argc, char **argv) {

  get_local_ip();

  /*code for catching ctrl-c signal so that we can free all the memory we malloced*/
   struct sigaction sigIntHandler;

   sigIntHandler.sa_handler = exit_handler;
   sigemptyset(&sigIntHandler.sa_mask);
   sigIntHandler.sa_flags = 0;

   sigaction(SIGINT, &sigIntHandler, NULL);

  int server_port;
  char *mode;
  char webMode = 0; //0 = false; 1 = true

  /*Parse Arguments*/
  if ((argc < 2) || (argc > 4)) {
    fprintf(stderr, "Syntax: %s port [mode] [root_directory]\n",argv[0]);
    return -1;
  }

  if (sscanf(argv[1], "%d", &server_port) < 1) {
    fprintf(stderr, "port should be numeric\n");
    return -1;
  }

  if (argc > 2) {
    mode = argv[2];
    int modelen = strlen(mode);
    if (modelen > 2) {
      if (strncmp(mode, "www", 3) == 0) {
        printf("Starting in web mode\n");
        webMode = 1;
      }
    }
  }

  if (argc > 3) {
    if (parse_root(argv[3]) < 0) {
      fprintf(stderr, "Invalid server root\n");
      return -1;
    }
  }

  /* socket and option variables */
  int sock, new_sock, max;
  int optval = 1;

  /* server socket address variables */
  struct sockaddr_in sin, addr;

  /* socket address variables for a connected client */
  socklen_t addr_len = sizeof(struct sockaddr_in);

  /* maximum number of pending connection requests */
  int BACKLOG = 5;

  /* variables for select */
  fd_set read_set, write_set;
  struct timeval time_out;
  int select_retval;

  /* number of bytes sent/received */
  int count;

  /* linked list for keeping track of connected sockets */
  struct node head;
  struct node *current, *next;
  root = &head;

  /* initialize dummy head node of linked list */
  head.socket = -1;
  head.next = 0;
  head.buffer = NULL;

  /* create a server socket to listen for TCP connection requests */
  if ((sock = socket (PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
      perror ("opening TCP socket");
      abort ();
    }
  
  /* set option so we can reuse the port number quickly after a restart */
  if (setsockopt (sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof (optval)) <0)
    {
      perror ("setting TCP socket option");
      abort ();
    }

  /* fill in the address of the server socket */
  memset (&sin, 0, sizeof (sin));
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = INADDR_ANY;
  sin.sin_port = htons (server_port);
  
  /* bind server socket to the address */
  if (bind(sock, (struct sockaddr *) &sin, sizeof (sin)) < 0)
    {
      perror("binding socket to address");
      abort();
    }

  /* put the server socket in listen mode */
  if (listen (sock, BACKLOG) < 0)
    {
      perror ("listen on socket failed");
      abort();
    }

  /* now we keep waiting for incoming connections,
     check for incoming data to receive,
     check for ready socket to send more data */
  while (1){

    /* set up the file descriptor bit map that select should be watching */
    FD_ZERO (&read_set); /* clear everything */
    FD_ZERO (&write_set); /* clear everything */

    FD_SET (sock, &read_set); /* put the listening socket in */
    max = sock; /* initialize max */

    /* put connected sockets into the read and write sets to monitor them */
    for (current = head.next; current; current = current->next) {
      FD_SET(current->socket, &read_set);

      if (current->pending_data) {
        /* there is data pending to be sent, monitor the socket
           in the write set so we know when it is ready to take more
           data */
        FD_SET(current->socket, &write_set);
      }

      if (current->socket > max) {
        /* update max if necessary */
        max = current->socket;
      }
    }

    time_out.tv_usec = 100000; /* 1-tenth of a second timeout */
    time_out.tv_sec = 0;

    /* invoke select, make sure to pass max+1 !!! */
    select_retval = select(max+1, &read_set, &write_set, NULL, &time_out);
    if (select_retval < 0){
      perror ("select failed");
      abort ();
    }

    if (select_retval == 0){
      /* no descriptor ready, timeout happened */
      continue;
    }
      
    if (select_retval > 0) /* at least one file descriptor is ready */{
      if (FD_ISSET(sock, &read_set)) /* check the server socket */{
        /* there is an incoming connection, try to accept it */
        new_sock = accept (sock, (struct sockaddr *) &addr, &addr_len);
              
        if (new_sock < 0){
                 perror ("error accepting connection");
                 abort ();
              }

        if (webMode) {
          service_request(&new_sock);
          continue;
        }

        /* make the socket non-blocking so send and recv will
           return immediately if the socket is not ready.
           this is important to ensure the server does not get
           stuck when trying to send data to a socket that
           has too much data to send already.
         */
        /* send and recv will return immediately if the socket is not ready.
         * This is quote very important.
         * If the socket is not ready, we need to mark receive or send in the queue.
         */
        if (fcntl (new_sock, F_SETFL, O_NONBLOCK) < 0){
          perror ("making socket non-blocking");
          abort ();
        }
  
        /* the connection is made, everything is ready */
        /* let's see who's connecting to us */
        printf("Accepted connection. Client IP address is: %s\n",
              inet_ntoa(addr.sin_addr));

        /* remember this client connection in our linked list */
        add(&head, new_sock, addr);
      }

      /* check other connected sockets, see if there is
         anything to read or some socket is ready to send
         more pending data */
      for (current = head.next; current; current = next) {
#ifdef DEBUG
        printf("checking write.\n");
#endif
        next = current->next;

            /* see if we can now do some previously unsuccessful writes */
        if (FD_ISSET(current->socket, &write_set)) {
              /* the socket is now ready to take more data */
              /* the socket data structure should have information
                 describing what data is supposed to be sent next.
                 but here for simplicity, let's say we are just
                 sending whatever is in the buffer buf
               */
          /*the server knows the msg_size and the number bytes sent.*/
          count = send(current->socket, current->buffer + current->n_bytes, current->msg_size - current->n_bytes, MSG_DONTWAIT);
#ifdef DEBUG
          printf("send count is %d.\n", count);
#endif
          if (count < 0) {
            switch(errno){
            /* we are trying to dump too much data down the socket,
               it cannot take more for the time being 
               will have to go back to select and wait til select
               tells us the socket is ready for writing
            */
            case EAGAIN:
              printf("socket not ready for send.\n");
              break;
            default:
              perror("not an good errno. closing the connection...");
              /* connection is closed, clean up 
                 assuming a client is either waiting to write or waiting to read.
                 can't be both*/
              close(current->socket);
              dump(&head, current->socket);
              break;
            }//switch(errno)
          }//if (count < 0)
          else{ // at least sent something, update the number bytes sent
            current->n_bytes += count;
          }


          /* note that it is important to check count for exactly
             how many bytes were actually sent even when there are
             no error. send() may send only a portion of the buffer
             to be sent.
           */
          if(current->n_bytes == current->msg_size){
            current->pending_data = 0;// stop sending the current message to this socket in the next iteration.
            current->n_bytes = 0;
          }
#ifdef DEBUG
          else if(current->n_bytes > current->msg_size){
            current->pending_data = 0;
            current->n_bytes = 0;
            printf("sent more bytes than needed, or error in tracking the number of bytes sent.\n");
          }
#endif
          else{
            current->pending_data = 1;
          }

        } //if (FD_ISSET(current->socket, &write_set)

        /*if there is data to receive from the client*/
        if (FD_ISSET(current->socket, &read_set)) {
#ifdef DEBUG
          printf("checking read.\n");
          /* we have data from a client */
          printf("number of bytes is %d.\n", current->n_bytes);
#endif
          if(current->msg_size == 0){
            count = recv(current->socket, current->buffer + current->n_bytes, BUF_LEN - current->n_bytes, MSG_DONTWAIT);
          }
          else{
            count = recv(current->socket, current->buffer + current->n_bytes, current->msg_size - current->n_bytes, MSG_DONTWAIT);
          }
#ifdef DEBUG
          printf("recv count is %d.\n", count);
#endif

          if (count == 0) {
              printf("Client closed connection. Client IP address is: %s\n", inet_ntoa(current->client_addr.sin_addr));
              /* connection is closed, clean up */
              close(current->socket);
              dump(&head, current->socket);
          }
          else if (count < 0){
            switch(errno){
            /*do nothign, wait for the next iteration to deal with this message receiving.
              Again, assuming that it has to be either read or write.
             */
            case EAGAIN:
              printf("socket not ready for recv.\n");
              break;
            default:
              perror("error receiving from a client, closed down the connecion");
              /* connection is closed, clean up */
              close(current->socket);
              dump(&head, current->socket);
              break;
            }//switch
          }// if count <= 0
          else {// count > 0
            /* we got count bytes of data from the client */
            current->n_bytes += count;
            if(current->n_bytes > 2){
              /*first time got more that 2 bytes, now the server knows the message size*/
              if(current->msg_size == 0){
                current->msg_size = ntohs(*(unsigned short *)current->buffer);
              }

              /*The server must have known the msg_size at this point.*/
              if(current->n_bytes == current->msg_size){ // prepare for sending the pong message
                current->pending_data = 1;
                current->n_bytes = 0;
              }//else do nothing, not ready to send to this socket yet.
#ifdef DEBUG
              else if(current->n_bytes > current->msg_size){
                current->pending_data = 1;
                current->n_bytes = 0;
                printf("received more data than the amount client has sent, or error in tracking number of bytes received.\n");
              }
#endif
            }//else do nothing
          } // we at least get something.
        } // if FD_ISSET read
      }//iterate through sockets
    }// if sellect
  }//while
}
