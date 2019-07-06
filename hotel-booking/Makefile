 #
 # @project:        hotel-booking
 # @file:           Makefile
 # @author(s):      Francesco Urbani <https://urbanij.github.io/>
 #
 # @date:           Fri Jun  7 16:47:28 CEST 2019
 # @Description:    Makefile
 #




all: server client

HEADERS=utils.h config.h Address.h Booking.h Hotel.h User.h 


EXTENSION :=
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
	EXTENSION:=_lin
endif
ifeq ($(UNAME_S),Darwin)
	EXTENSION:=_osx
endif



LINKING_LIB :=
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
	LINKING_LIB:=-lpthread -lcrypt
endif
ifeq ($(UNAME_S),Darwin)
	LINKING_LIB:=
endif



server.o: server.c $(HEADERS)
	gcc -c -Wall server.c -o server.o
server: server.o
	gcc server.o -o server$(EXTENSION) $(LINKING_LIB) -lsqlite3

client.o: client.c $(HEADERS)
	gcc -c -Wall client.c -o client.o
client: client.o
	gcc client.o -o client$(EXTENSION) $(LINKING_LIB)





clean:
	rm *.o client_* server_*
