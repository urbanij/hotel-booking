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
LINKING_LIB :=
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
	LINKING_LIB:=-lpthread -lcrypt
endif
ifeq ($(UNAME_S),Darwin)
	LINKING_LIB:=
endif



server: server.c $(HEADERS)
	gcc -Wall server.c -o server$(EXTENSION) $(LINKING_LIB) -lsqlite3 -g3 -std=c99

client: client.c $(HEADERS)
	gcc -Wall client.c -o client$(EXTENSION) $(LINKING_LIB) -std=c99





clean:
	rm -rf *.dSYM 	# gdb
	rm -f client_* 
	rm -f server_*   

wipe:
	rm -rf .data 
	rm -rf *.dSYM 	# gdb
	rm -f client_* 
	rm -f server_*


