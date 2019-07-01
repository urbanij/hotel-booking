/**
 * @project:        hotel-booking
 * @file:           server.c
 * @author(s):      Francesco Urbani <https://urbanij.github.io/>
 *
 * @date:           Mon Jul  1 12:31:31 CEST 2019
 * @Description:    server side
 *
 */

#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <pthread.h>
#ifndef __APPLE__
    #include <crypt.h>
#endif
#include <pthread.h>

#include "xp_sem.h"


// config definition and declarations
#include "config.h"

// constants and definitions
#include "utils.h"

// data structures and relative functions
#include "Address.h"
#include "Booking.h"
#include "Hotel.h"
#include "User.h"


/********************************/
/*                              */
/*      custom data types       */
/*                              */
/********************************/


/********************************/
/*                              */
/*          global var          */
/*                              */
/********************************/



static xp_sem_t     lock_g;                      // global lock


static xp_sem_t     free_threads;               // semaphore for waiting for free threads
static xp_sem_t     evsem[NUM_THREADS];         // per-thread event semaphores

static int          fds[NUM_THREADS];           // array of file descriptors


static int          busy[NUM_THREADS];          // map of busy threads
static int          tid[NUM_THREADS];           // array of pre-allocated thread IDs
static pthread_t    threads[NUM_THREADS];       // array of pre-allocated threads

// static Booking      bookings[NUM_THREADS];


/*                       __        __
 *     ____  _________  / /_____  / /___  ______  ___  _____
 *    / __ \/ ___/ __ \/ __/ __ \/ __/ / / / __ \/ _ \/ ___/
 *   / /_/ / /  / /_/ / /_/ /_/ / /_/ /_/ / /_/ /  __(__  )
 *  / .___/_/   \____/\__/\____/\__/\__, / .___/\___/____/
 * /_/                             /____/*/   

// void help               ();
// void register           (int sockfd);
// void login              (int sockfd);
// void view               (int sockfd);

/**
 * thread handler function
 */
void*   threadHandler   (void* opaque);

/**
 * command dispatcher: runs inside the threadHandler functions 
 * and dispatches the inbound commands to the executive functions.
 */
void    dispatcher      (int sockfd, int thread_index);


/********************************/
/*                              */
/*          functions           */
/*                              */
/********************************/



int main(int argc, char** argv)
{
    int conn_sockfd;    // connected socket file descriptor


    // reading arguments from stdin
    Address address = readArguments(argc, argv);

    // setup the server and return socket file descriptor.
    int sockfd = setupServer(&address);         // listening socket file descriptor

    

    // setup semaphores
    xp_sem_init(&lock_g, 0, 1);          // init to 1 since it's binary


    char ip_client[INET_ADDRSTRLEN];    // no idea...
    



    // initialize lock_g


    xp_sem_init(&free_threads, 0, NUM_THREADS);


    // building pool
    for (int i = 0; i < NUM_THREADS; i++) {
        int rv;

        tid[i] = i;
        rv = pthread_create(&threads[i], NULL, threadHandler, (void*) &tid[i]);
        if (rv) {
            printf("ERROR: #%d\n", rv);
            exit(-1);
        }
        busy[i] = 0;                    // busy threads        initialized at 0
        xp_sem_init(&evsem[i], 0, 0);   // semaphore of events initialized at 0
    }




    while(1)
    {
        int thread_index;

        xp_sem_wait(&free_threads);             // wait until a thread is free

        struct sockaddr_in client_addr;         // client address
        socklen_t addrlen = sizeof(client_addr);

        conn_sockfd = accept(sockfd, (struct sockaddr*) &client_addr, &addrlen);
        if (conn_sockfd < 0) {
            perror_die("accept()");
        }

        // conversion: network to presentation
        inet_ntop(AF_INET, &client_addr.sin_addr, ip_client, INET_ADDRSTRLEN);

        printf("%s: \x1b[32mconnection established\x1b[0m  with client @ %s:%d\n",
                                                    __func__, ip_client, client_addr.sin_port 
                                                    );



        // look for free thread

// lock_g acquisition -- critical section
        xp_sem_wait(&lock_g);

        for (thread_index = 0; thread_index < NUM_THREADS; thread_index++){  //assegnazione del thread
            if (busy[thread_index] == 0) {
                break;
            }
        }

        printf("%s: Thread #%d has been selected.\n", __func__, thread_index);

        fds[thread_index] = conn_sockfd;    // assigning socket file descriptor to the file descriptors array
        busy[thread_index] = 1;             // notification busy thread, so that it can't be assigned to another, till its release

// lock_g release -- end critical section
        xp_sem_post(&lock_g);

        
        xp_sem_post(&evsem[thread_index]);        

    }

    // pthread_exit(NULL);



    close(conn_sockfd);


    return 0;
}




/**
 * Thread body for the request handlers. Wait for a signal from the main
 * thread, collect the client file descriptor and serve the request.
 * Signal the main thread when the request has been finished.
 */
void* threadHandler(void* indx)
{
    int thread_index = *(int*) indx;       // unpacking argument
    int conn_sockfd;                       // file desc. local variable

    printf("THREAD #%d ready.\n", thread_index);

    while(1)
    {
        
        // waiting for a request assigned by the main thread
        xp_sem_wait(&evsem[thread_index]);
        printf ("Thread #%d took charge of a request\n", thread_index);

    // critical section
        xp_sem_wait(&lock_g);
        conn_sockfd = fds[thread_index];
        xp_sem_post(&lock_g);
    // end critical section


        // keep reading messages from client until "quit" arrives.
        
        // serving the request (dispatched)
        dispatcher(conn_sockfd, thread_index);
        printf ("Thread #%d served a request\n", thread_index);


        // notifying the main thread that THIS thread is now free and can be assigned to new requests.
    // critical section
        xp_sem_wait(&lock_g);
        busy[thread_index] = 0;
        xp_sem_post(&lock_g);
    // end critical section
        
        xp_sem_post(&free_threads);

        

        #if 0
            if (strcmp(command, "quit") == 0)
            {
                // quit(thread_index);
                printf("%s\n", "quit func called");
            }
        #endif


    }


    pthread_exit(NULL);
    

}


/** 
 * actually serving the requests of the client.
 *
 */
void dispatcher (int conn_sockfd, int thread_index){

    char command[BUFSIZE];
    Booking booking;    // variable to handle the functions 
                        // `release` and `reserve` from the client.


    while (1) 
    {
    
        #if 1
            // clean the pipes after receiveing each command.
            memset(command,      '\0', sizeof(command));
            memset(booking.date, '\0', sizeof(booking.date));
            memset(booking.code, '\0', sizeof(booking.code));
        #endif

        readSocket(conn_sockfd, command);  // fix space separated strings
        
        printf("THREAD #%d: command received: %s\n", thread_index, command);




        if (strcmp(command, "res") == 0){                   // got reserve
            readSocket(conn_sockfd, booking.date);
        }
        else if (strcmp(command, "rel") == 0){              // got release
            // if i'm receveing release i also want to read
            // the remaining arguments it carries.

            char room_string[6]; // will store the room prior to be converted to integer.
            readSocket(conn_sockfd, booking.date);
            readSocket(conn_sockfd, room_string);
            booking.room = atoi(room_string);
            readSocket(conn_sockfd, booking.code);

            printf("Reservation #%s on %s, room %d\n", 
                booking.code, booking.date, booking.room);
        }
        
        else if (strcmp(command, "q") == 0){
            printf("%s\n", "quitting");     // continue here....
            break;
        }

         
            

    }



    return;
}



