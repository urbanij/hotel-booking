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
void*   threadHandler           (void* opaque);

/**
 * command dispatcher: runs inside the threadHandler functions 
 * and dispatches the inbound commands to the executive functions.
 */
void    dispatcher              (int sockfd, int thread_index);

/**
 * takes a username `u` and opens the file 
 * `users.txt` to see if such username is
 * therein contained.
 * returns 0 if it's not contained, -1 otherwise.
 */
int     usernameIsAvailable     (char* u);


/**
 *
 */
int     updateUsersRecordFile   (char*, char*);

/**
 *
 */
char*   encryptPassword         (char* password);

/**
 *
 */
void    makeSalt                (char* salt);

/**
 *
 */
int     checkIfUsernameAlreadyRegistered();

/**
 *
 */
int     checkIfPasswordMatches();









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

/* . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . */


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

/* . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . */

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



    User* user = (User*) malloc(sizeof(User));
    memset(user->username, '\0', sizeof(user->username));
    memset(user->actual_password, '\0', sizeof(user->actual_password));

    while (1) 
    {
    

        // clean the pipes after receiveing each command.
        memset(command,      '\0', BUFSIZE);
        memset(booking.date, '\0', sizeof(booking.date));
        memset(booking.code, '\0', sizeof(booking.code));
    
        

        


        int rv;

        switch (state)
        {
            case INIT:
            // do stuff
                readSocket(conn_sockfd, command);  // fix space separated strings
                printf("THREAD #%d: command received: %s\n", thread_index, command);

            // update FSM
                if      (strcmp(command, "h") == 0)  // help
                    state = HELP_UNLOGGED;
                else if (strcmp(command, "r") == 0)  // register
                    state = REGISTER;
                else if (strcmp(command, "l") == 0)  // login
                    state = LOGIN_REQUEST;
                else if (strcmp(command, "q") == 0)  // quit
                    state = QUIT;
                else
                    state = INIT;
                break;
    

            case HELP_UNLOGGED:
            // do stuff
                writeSocket(conn_sockfd, "H");
            // update FSM
                state = INIT;
                break;



            case REGISTER:
            // do stuff
                printf("%s\n", "400 received register.");
                writeSocket(conn_sockfd, "Choose username: "); // works

            // update FSM
                state = PICK_USERNAME;
                break;

            case PICK_USERNAME:
            // do stuff
                readSocket(conn_sockfd, command);
                strcpy(user->username, command);

                printf("usernameee %s\n", user->username); // works

            // update FSM

                rv = usernameIsAvailable(user->username);
                if (rv == 0){
                    state = PICK_PASSWORD;
                    writeSocket(conn_sockfd, "Y");  // Y stands for: "username OK.\nChoose password: "
                }
                else {
                    state = PICK_USERNAME;
                    writeSocket(conn_sockfd, "N");  // N stands for: "username already taken, pick another one: "
                }

                break;

            case PICK_PASSWORD:
            // do stuff
                readSocket(conn_sockfd, command);
                strcpy(user->actual_password, command);

                printf("passworrrd %s\n", user->actual_password);

            // update FSM
                state = SAVE_CREDENTIAL;
                break;

            case SAVE_CREDENTIAL:
            // do stuff
                // password has to be hashed before storing it.
                updateUsersRecordFile(user->username, encryptPassword(user->actual_password));
                writeSocket(conn_sockfd, "OK: Account was successfully setup.");
                writeSocket(conn_sockfd, "Successfully registerd, you are now logged in."); // works

            // update FSM
                state = LOGIN;
                break;

            case LOGIN_REQUEST:
            // do stuff
                writeSocket(conn_sockfd, "Insert username: ");
            // update FSM
                state = CHECK_USERNAME;
                break;

            case CHECK_USERNAME:
            // do stuff
                readSocket(conn_sockfd, command);
                rv = checkIfUsernameAlreadyRegistered(command);
                if (rv == 0){
                    state = CHECK_PASSWORD;
                    writeSocket(conn_sockfd, "Y");  // Y stands for OK
                }
                else {
                    state = INIT;
                    writeSocket(conn_sockfd, "N");  // N stands for NOT OK
                }
            // update FSM
                
                break;

            case CHECK_PASSWORD:
            // do stuff
                readSocket(conn_sockfd, command);
                rv = checkIfPasswordMatches(command);
                if (rv == 0){
                    state = GRANT_ACCESS;
                }
                else {
                    state = INIT;
                    writeSocket(conn_sockfd, "N");  // N stands for NOT OK
                }
            // update FSM
                
                break;

            case GRANT_ACCESS:
            // do stuff
                writeSocket(conn_sockfd, "Y");  // Y stands for OK
            // update FSM
                state = LOGIN;
                break;

            case LOGIN:
            // do stuff
                
                readSocket(conn_sockfd, command);  // fix space separated strings
                printf("THREAD #%d: command received: %s\n", thread_index, command);

                if      (strcmp(command, "hh") == 0)  // help
                    state = HELP_LOGGED_IN;
                else if (strcmp(command, "q") == 0)  // quit
                    state = QUIT;
                else if (strcmp(command, "logout") == 0) // logout
                    state = INIT;
                else
                    state = INIT;
                break;
            
            // update FSM

                break;

            case HELP_LOGGED_IN:
            // do stuff
                writeSocket(conn_sockfd, "H");
            // update FSM
                state = LOGIN;
                break;


            
            case QUIT:
            // do stuff
                free(state_pointer);
                
                free(user);
    
                
                printf("%s\n", "quitting");     // continue here....
                goto ABORT;
            // update FSM

            default:
                break;

        }
        
        
        printServerFSMState(&state, &thread_index);
            
    }

ABORT:
    return;
}


/* . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . */




int usernameIsAvailable(char* u){
    char username[30];
    char enc_pass[30];
    char line[50];

    FILE* users_file;

    users_file = fopen(USER_FILE, "r");
    if(users_file == NULL) {
        return 0;       // no file exists, hence no username exists, gr8
    }

    // check whether the user is already in the file (= already registered)
    while(fgets(line, sizeof(line), users_file)) {
        sscanf(line, "%s %s\n", username, enc_pass);
        
        // checking whether `u` (i.e. username coming from the client) 
        // matches `username` (i.e. username saved on the file in the server)
        if (strcmp(u, username) == 0){
            fclose(users_file);
            return -1;  // found user in the file
        }
    }
    return 0;           // username `u` is new to the system, hence the registration can proceed.
}




int updateUsersRecordFile(char* username, char* encrypted_password){
    
    FILE* users_file = fopen(USER_FILE, "a+");
    if(users_file == NULL) {
        perror_die("fopen() @ server.c:569");
    }

    // buffer (will) store the line to be appended to the file.
    char buffer[50];
    memset(buffer, '\0', sizeof(buffer));

    // creating the "payload"
    strcat(buffer, username);
    strcat(buffer, "\t\t");
    strcat(buffer, encrypted_password);

    // add line
    fprintf(users_file, "%s\n", buffer);

    // close connection to file
    fclose(users_file);

    return 0;
}

char* encryptPassword(char* password){
    // encrypted password returned 
    // (static because it has to outlive the time-scope of the function)
    static char res[512];   
    
    char salt[2];           // salt

    makeSalt(salt);         // making salt

    // encrypt password
    strncpy(res, crypt(password, salt), sizeof(res)); 
    //             ^--- CRYPT(3)    BSD Library Functions Manual

    return res;
}


void makeSalt(char* salt) {

    int r;

    // init random number generator
    srand(time(NULL));
    
    // make sure salt chars are actual characters, not than special ASCII symbols
    r = rand();
    r = r % ('z' - '0');
    salt[0] = '0' + r;
    
    r = rand();
    r = r % ('z' - '0');
    salt[1] = '0' + r;
    
    #if DEBUG
        printf("Salt: %c%c\n", salt[0], salt[1]);
    #endif
}

    





int checkIfUsernameAlreadyRegistered(){
    return 0;
}
int checkIfPasswordMatches(){
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





int checkIfFull(){
    return 0;
}
int checkIfValidEntry(){
    return 0;
}

