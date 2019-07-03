/**
 * @project:        hotel-booking
 * @file:           server.c
 * @author(s):      Francesco Urbani <https://urbanij.github.io/>
 *
 * @date:           Mon Jul  1 12:31:31 CEST 2019
 * @Description:    server side
 *
 * 
 * @compilation:    `make server` or `gcc server.c -o server [-lcrypt -lpthread]`
 *
 * LOC: cat server.c | sed '/^\s*$/d' | wc -l
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

#define HELP_UNLOGGED_MESSAGE_1 "Commands:\n\
        \x1b[36m help     \x1b[0m --> show commands\n\
        \x1b[36m register \x1b[0m --> register an account\n\
        \x1b[36m login    \x1b[0m --> log into the system\n\
        \x1b[36m quit     \x1b[0m --> log out and quit.\n\
        \x1b[36m reserve  \x1b[0m --> book a room             (log-in required)\n\
        \x1b[36m release  \x1b[0m --> cancel a booking        (log-in required)\n\
        \x1b[36m view     \x1b[0m --> show current bookings   (log-in required)\n"

#define HELP_LOGGED_IN_MESSAGE_1 "Commands:\n\
        \x1b[36m help     \x1b[0m --> show commands\n\
        \x1b[36m reserve  \x1b[0m --> book a room\n\
        \x1b[36m release  \x1b[0m --> cancel a booking\n\
        \x1b[36m view     \x1b[0m --> show current bookings\n\
        \x1b[36m quit     \x1b[0m --> log out and quit.\n\
        \x1b[36m register \x1b[0m --> register an account     (you have to be logged-out)\n\
        \x1b[36m login    \x1b[0m --> log into the system     (you have to be logged-out)\n"

#define HELP_UNLOGGED_MESSAGE_2 "Commands:\n\
        \x1b[36m help     \x1b[0m --> show commands\n\
        \x1b[36m register \x1b[0m --> register an account\n\
        \x1b[36m login    \x1b[0m --> log into the system\n\
        \x1b[36m quit     \x1b[0m --> log out and quit.\n"

#define HELP_LOGGED_IN_MESSAGE_2 "Commands:\n\
        \x1b[36m help     \x1b[0m --> show commands\n\
        \x1b[36m reserve  \x1b[0m --> book a room\n\
        \x1b[36m release  \x1b[0m --> cancel a booking\n\
        \x1b[36m view     \x1b[0m --> show current bookings\n\
        \x1b[36m quit     \x1b[0m --> log out and quit.\n"

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



static xp_sem_t     lock_g;                     // global lock


static xp_sem_t     free_threads;               // semaphore for waiting for free threads
static xp_sem_t     evsem[NUM_THREADS];         // per-thread event semaphores

static int          fds[NUM_THREADS];           // array of file descriptors


static int          busy[NUM_THREADS];          // map of busy threads
static int          tid[NUM_THREADS];           // array of pre-allocated thread IDs
static pthread_t    threads[NUM_THREADS];       // array of pre-allocated threads

// static server_fsm_state_t  state[NUM_THREADS];         // FSM states



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
void*       threadHandler   (void* opaque);

/**
 * command dispatcher: runs inside the threadHandler functions 
 * and dispatches the inbound commands to the executive functions.
 */
void        dispatcher      (int sockfd, int thread_index);

// FSM related
/**
 *
 */
server_fsm_state_t* updateServerFSM       (server_fsm_state_t* state, char* command);


/**
 *
 */
int usernameIsAvailable();







/**
 *
 */
int checkUsername();

/**
 *
 */
int checkPassowrd();

/**
 *
 */
int checkIfLoggedIn();

/**
 *
 */
int checkIfUsernameAlreadyRegistered();

/**
 *
 */
int checkIfFull();

/**
 *
 */
int checkIfValidEntry();

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

    // initialized FSM
    #if 0
    for (int i = 0; i < NUM_THREADS; i++){
        state[i] = INIT;
    }
    #endif

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
        printf ("Thread #%d closed session, client disconnected.\n", thread_index);


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
    Booking booking;        // variable to handle the functions 
                            // `release` and `reserve` from the client.


    /* creating dinamic variable to hold the value of the current state
     * on the server-side FSM
     */

    // create pointer variable called `state_pointer`
    server_fsm_state_t* state_pointer = (server_fsm_state_t*) malloc(sizeof(server_fsm_state_t));

    // `state pointer` is not actually used throughout this function but its 
    // value is dereferenced to the variable `state`.
    // `state_pointer` is used again when freeing the memory at the end of this routine.
    server_fsm_state_t state = *state_pointer;



    while (1) 
    {
    
        #if 1
            // clean the pipes after receiveing each command.
            memset(command,      '\0', BUFSIZE);
            memset(booking.date, '\0', sizeof(booking.date));
            memset(booking.code, '\0', sizeof(booking.code));
        #endif
        
        
        #if 0
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
        #else

        

        #if 1
        switch (state)
        {
            case INIT:
                readSocket(conn_sockfd, command);  // fix space separated strings
                printf("THREAD #%d: command received: %s\n", thread_index, command);
                break;

            case HELP_UNLOGGED:
                // printf(HELP_UNLOGGED_MESSAGE_2);
                writeSocket(conn_sockfd, HELP_UNLOGGED_MESSAGE_2);
                break;



            case REGISTER:
                printf("%s\n", "400 received register.");
                writeSocket(conn_sockfd, "Choose username: "); // works
                break;

            case PICK_USERNAME:
                readSocket(conn_sockfd, command);
                printf("usernameee %s\n", command); // works

                writeSocket(conn_sockfd, "username OK.");   // EDIT THIS!

                break;

            case PICK_PASSWORD:
                readSocket(conn_sockfd, command);
                printf("passworrrd %s\n", command);
                break;

            case SAVE_CREDENTIAL:
                writeSocket(conn_sockfd, "OK: Account was successfully setup.");
                break;

            case LOGIN:
                writeSocket(conn_sockfd, "Successfully registerd, you are now logged in."); // works
                
                readSocket(conn_sockfd, command);  // fix space separated strings
                printf("THREAD #%d: command received: %s\n", thread_index, command);
                break;


            
            case QUIT:
                free(state_pointer);
                printf("%s\n", "quitting");     // continue here....
                goto ABORT;

            default:
                break;

        }
        #endif


        updateServerFSM(&state, command);

        
        
        printServerFSMState(&state, &thread_index);
            
        #endif
    }

ABORT:
    return;
}





server_fsm_state_t* updateServerFSM(server_fsm_state_t* state, char* command)
{
    int rv; 
    
    switch (*state)
    {

        case INIT:
            if      (strcmp(command, "h") == 0)  // help
                *state = HELP_UNLOGGED;
            // else if (strcmp(command, "v") == 0)  // view
            //     *state = INIT;
            else if (strcmp(command, "r") == 0)  // register
                *state = REGISTER;
            // else if (strcmp(command, "l") == 0)  // login
            //     *state = CHECK_USERNAME;
            else if (strcmp(command, "q") == 0)  // quit
                *state = QUIT;
            else
                *state = INIT;
            break;
            
        case HELP_UNLOGGED:
            *state = INIT;
            break;


    
        case REGISTER:
            *state = PICK_USERNAME;
            break;

        case PICK_USERNAME:
            rv = usernameIsAvailable();
            if (rv == 0){
                *state = PICK_PASSWORD;
            }
            else {
                *state = PICK_USERNAME;
            }
            break;

        case PICK_PASSWORD:
            *state = SAVE_CREDENTIAL;
            break;
    
        case SAVE_CREDENTIAL:
            *state = LOGIN;
            break;
        
        case LOGIN:

            break;

        case QUIT:
            *state = INIT;  // makes no sense because it quits anyway.
            break;








        #if 0
        case LOGIN:
            if      (strcmp(command, "h") == 0)  // help
                *state = HELP_LOGGED_IN;
            else if (strcmp(command, "v") == 0)  // view
                *state = VIEW;
            else if (strcmp(command, "res") == 0)  // reserve
                *state = CHECK_IF_FULL;
            else if (strcmp(command, "rel") == 0)  // release
                *state = CHECK_IF_VALID_ENTRY;
            else if (strcmp(command, "q") == 0)  // quit
                *state = QUIT;
            break;


        case HELP_LOGGED_IN:
            *state = LOGIN;
            break;

        case CHECK_IF_FULL:
            rv = checkIfFull();
            if (rv == 0){
                *state = RESERVE;
            }
            else {
                *state = LOGIN;
            }
            break;

        case RESERVE:
            *state = LOGIN;
            break;

        case CHECK_IF_VALID_ENTRY:
            rv = checkIfValidEntry();
            if (rv == 0){
                *state = RELEASE;
            }
            else {
                *state = LOGIN;
            }
            break;
        #endif

        
        

        default:
            *state = INIT;
    }
    
    return  state;
}


int usernameIsAvailable(){
    return 0;
}








int checkUsername(){
    return -1;
}
int checkPassowrd(){
    return -1;
}
int checkIfLoggedIn(){
    return 0;
}



int checkIfUsernameAlreadyRegistered(){
    return 0;
}

int checkIfFull(){
    return 0;
}
int checkIfValidEntry(){
    return 0;
}

