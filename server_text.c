#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>
#include <sys/time.h>
#include <errno.h>

#define NUM_OPT     4
#define SIZE_WIDTH  2
#define TIME_WIDTH1 4
#define TIME_WIDTH2 4
#define MSG_MIN     10
#define MSG_MAX     65535
#define NUM_MIN     1
#define NUM_MAX     10000
#define EXTRA_WIDTH (SIZE_WIDTH + TIME_WIDTH1 + TIME_WIDTH2)
#define DATA_SIZE (msg_size - EXTRA_WIDTH)

#define PRINT_OPTS() do{\
    perror("Expected format of commandline:\n"\
           "./client hostname port size count.\n\n"\
           "hostname: The domain name or the ip address of the host where the server is running.\n"\
           "port    : The port on which the server is running, in the range of [18000,18200].\n"\
           "size    : The size in bytes of each message to send, in the range of [10, 65,535].\n"\
           "count   : The number of messages exchanges to perform, in the range of [1, 10,000].\n"\
          );\
}while(0)


/* simple client, takes two parameters, the server domain name,
   and the server port number */

int main(int argc, char** argv) {
  long total_sec = 0;
  long total_usec = 0;

  /*used for time stamping the send and receives.*/
  struct timeval time_stamp;

  if(argc != NUM_OPT + 1){
    PRINT_OPTS();
    abort();
  }
  /* our client socket */
  int sock;

  /* address structure for identifying the server */
  struct sockaddr_in s_in;

  /* convert server domain name to IP address */
  struct hostent *host = gethostbyname(argv[1]);
  if(!host){
    perror("invalid host name.\n");
    PRINT_OPTS();
    abort();
  }
  unsigned int server_addr = *(unsigned int *) host->h_addr_list[0];

  /* server port number */
  unsigned short server_port = atoi (argv[2]);
  
  unsigned short msg_size;
  unsigned short num_messages;

  long ms = atol (argv[3]);
  long mn = atol (argv[4]);

  /* check input numbers */
  if (ms < MSG_MIN || 
      ms > MSG_MAX || 
      mn < NUM_MIN || 
      mn > NUM_MAX) {
     perror("\nError: inappropriate message size or number.\n");
     PRINT_OPTS();
     abort();
  } else {
    msg_size = (unsigned short) ms;
    num_messages = (unsigned short) mn;
  }

#ifdef DEBUG
  printf("msg_size is %d\n"
         "num_messages is %d\n", msg_size, num_messages);
#endif //DEBUG

  /* allocate a memory buffer in the heap */
  /* putting a buffer on the stack like:

         char buffer[500];

     leaves the potential for
     buffer overflow vulnerability */
  char *msg_buffer = (char*) malloc(msg_size * sizeof(char));
  if (!msg_buffer)
    {
      perror("failed to allocated buffer");
      abort();
    }

  /* create a socket */
  if ((sock = socket (PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
      perror ("opening TCP socket");
      abort ();
    }

  /* fill in the server's address */
  memset (&s_in, 0, sizeof (s_in));
  s_in.sin_family = AF_INET;
  s_in.sin_addr.s_addr = server_addr;
  s_in.sin_port = htons(server_port);

  /* connect to the server */
  if (connect(sock, (struct sockaddr *) &s_in, sizeof (s_in)) < 0)
    {
      perror("connect to server failed");
      abort();
    }

#ifdef TEST_TEXT
  /*make the message*/
  /*read message*/
  /*There should be randome data in the buffer now.*/
  char* data_buffer = msg_buffer + EXTRA_WIDTH;
  printf("Please type in message:\n");
  fgets(data_buffer, data_size, stdin);

  /*terminate the message with '0'*/
  data_buffer[data_size-1] = 0;
  data_buffer[strlen(data_buffer)-1] = 0;
  data_size = strlen(data_buffer);
  msg_size = data_size + EXTRA_WIDTH;

  printf("string length is %d.\n", strlen(data_buffer));
  printf("This is the message the client reads:\n%s.\n", data_buffer);
#endif// TEST_TEXT

  *(unsigned short*)msg_buffer = htons(msg_size);
  int count = 0;
  while (count < num_messages) {

#ifdef DEBUG
    printf("number of iteration = %d.\n", count);
#endif

    /*get time before send*/
    gettimeofday(&time_stamp, NULL);
    
    /*setup send message, message size and the data_buffer has been set*/
    *(unsigned int*)(msg_buffer + SIZE_WIDTH) = ntohl(time_stamp.tv_sec);
    *(unsigned int*)(msg_buffer + SIZE_WIDTH + TIME_WIDTH1) = ntohl(time_stamp.tv_usec);

    /*send the ping message*/
    int buffer_end = msg_buffer + msg_size;
    int remain_len = msg_size;
    char* buffer = msg_buffer;
    int tmp_count;
    while(buffer < buffer_end){

#ifdef DEBUG
      printf("%d bytes remaining.\n", remain_len);
#endif

      tmp_count = send(sock, buffer, remain_len, 0);

#ifdef DEBUG
      printf("send count is %d.\n", tmp_count);
#endif

      if(tmp_count < 0){
        if(errno == EAGAIN)
          continue;
        else if (errno == EWOULDBLOCK)
          continue;
        else{
          sleep(0.0001);
          continue;
        }
        perror("error sending to server...\n");
        free(msg_buffer);
        abort();
      }
      else if(tmp_count == 0){
        continue;
        /*what if the client receive returns 0?*/
        printf("send returns 0.\n");
        free(msg_buffer);
        abort();
      }
      buffer     +=  tmp_count;
      remain_len += -tmp_count;
    }

    /*receive the pong message*/
    buffer = msg_buffer;
    remain_len = buffer_end - (int)buffer;
    while(buffer < buffer_end){

#ifdef DEBUG
      printf("%d bytes remaining.\n", remain_len);
#endif

      tmp_count = recv(sock, buffer, remain_len, 0);

#ifdef DEBUG
      printf("recv count is %d.\n", tmp_count);
#endif

      if(tmp_count < 0){

#ifdef DEBUG
        printf("some error in receiving. try again.\n");
        continue;
#endif

        if(errno == EAGAIN)
          continue;
        else if (errno == EWOULDBLOCK)
          continue;
        printf("error code: %d.\n", errno);
        perror("error receiving from server...");
        free(msg_buffer);
        abort();
      }
      else if(tmp_count == 0){
        perror("connection closed...");
        free(msg_buffer);
        abort();
        /*what if the client receive returns 0?*/
      }
      buffer     +=  tmp_count;
      remain_len += -tmp_count;
    }
    
    gettimeofday(&time_stamp, NULL);
    total_sec  += time_stamp.tv_sec  - ntohl(*(unsigned int*)(msg_buffer + SIZE_WIDTH));
    total_usec += time_stamp.tv_usec - ntohl(*(unsigned int*)(msg_buffer + SIZE_WIDTH + TIME_WIDTH1));
    count++;
  }

  printf("The average latency is %.3fms.\n", (total_sec * 1000 + total_usec/1000.0)/num_messages);
  free(msg_buffer);
  return 0;
}
