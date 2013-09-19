OBJS = httpd.o server_text.o
LDLIBS = -lmagic

CC = gcc
CFLAGS = -g -Wall -O2
MAKE = make

CLIENT = client
SERVER = server

all: $(CLIENT) $(SERVER)

$(CLIENT): client_text.c
	${CC} ${CFLAGS} client_text.c -o $(CLIENT)

$(SERVER): $(OBJS) magic
	${CC} ${CFLAGS} ${OBJS} ${LDLIBS} -Llibmagic -o $(SERVER)

magic:
	$(MAKE) -C libmagic

clean:
	rm -rf $(CLIENT) $(SERVER) $(OBJS)
    
    

