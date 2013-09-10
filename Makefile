OBJS = httpd.o server_text.o
LDLIBS = -lmagic

CC = gcc
CFLAGS = -g -Wall -O2 -Llibmagic
MAKE = make

CLIENT = client
SERVER = server

all: $(CLIENT) $(SERVER)

$(CLIENT):
	${CC} ${CFLAGS} client_text.c -o $(CLIENT)

$(SERVER): $(OBJS) magic
	${CC} ${CFLAGS} ${OBJS} ${LDLIBS} -o $(SERVER)

magic:
	$(MAKE) -C libmagic

clean:
	rm -rf $(CLIENT) $(SERVER) $(OBJS)
    
    

