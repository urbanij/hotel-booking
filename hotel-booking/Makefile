 #
 # @name            hotel-booking
 # @file            Makefile
 # @authors         Francesco Urbani <https://urbanij.github.io/>
 #
 # @date            Fri Jun  7 16:47:28 CEST 2019
 # @brief           Makefile
 #


CC=gcc
CFLAGS=-Wall -g3

HEADERS=utils.h config.h Address.h Booking.h Hotel.h User.h 


# custom bin extension based on platform
EXTENSION :=
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
	EXTENSION:=_lin
# 	EXTENSION:=
endif
ifeq ($(UNAME_S),Darwin)
	EXTENSION:=_osx
# 	EXTENSION:=
endif


# custom libraries to link against based on plaftorm.
LIBS :=
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
	LIBS:=-lpthread -lcrypt -lsqlite3
endif
ifeq ($(UNAME_S),Darwin)
	LIBS:= -lsqlite3
endif



all: server client

server.o: server.c $(HEADERS)
	gcc -c $(CFLAGS) server.c -o server.o -g3 -pedantic

server: server.o
	gcc $(CFLAGS) server.o -o server$(EXTENSION) $(LIBS) -pedantic 

client.o: client.c $(HEADERS)
	gcc -c $(CFLAGS) client.c -o client.o -g3 -pedantic

client: client.o
	gcc $(CFLAGS) client.o -o client$(EXTENSION) -pedantic




doxy:
	doxygen && cd docs/latex && make


clean:
	rm -rf *.dSYM 	# gdb
	rm -f core
	rm -f client_* 
	rm -f server_*
	rm -f *.o
	rm -rf docs

wipe:	# wipes out database and users.txt 
	rm -rf .data 
	

