OBJS = httpd.o server_text.o

CC = gcc
CFLAGS = -g -Wall -O2

CLIENT = client
SERVER = server

all: $(CLIENT) $(SERVER)

$(CLIENT):
	${CC} ${CFLAGS} client_text.c -o $(CLIENT)

$(SERVER): $(OBJS)
	${CC} ${CFLAGS} ${OBJS} -o $(SERVER)

clean:
	rm -rf $(CLIENT) $(SERVER) $(OBJS)
    
    

