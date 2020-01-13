src = $(wildcard *.c)
obj = $(patsubst %.c, %.o, $(src))

all:server 

server: server.o wrap.o http.o
	gcc server.o wrap.o http.o -o server -Wall -lpthread
#client: client.o wrap.o
#	gcc client.o wrap.o -o client -Wall

%.o:%.c
	gcc -c $< -Wall -lpthread

.PHONY: clean all
clean: 
	-rm -rf server $(obj)
