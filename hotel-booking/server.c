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
 *
 *
 * TODO:            - checkDateValidity()
 *                  - assignRoom()
 *                  - use regex to check date validity
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

#include <sqlite3.h>
#include <regex.h>

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



#define QUOTE(...) #__VA_ARGS__     // this define allowes me to avoid writing every 
                                    // SQL command within quotes.
                                    // https://stackoverflow.com/a/17996915/6164816


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
int     usernameIsRegistered     (char* u);


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
int     checkIfPasswordMatches  (char*, char*);

/**
 *
 */
int     checkDateValidity       ();

/**
 *
 */
int     checkAvailability       ();

int     saveReservation         (char* u, char* d, char* r, char* c);


int     commitToDatabase(const char*);

int     setupDatabase();


char*   assignRoom();

char*   assignRandomReservationCode();
// . . . . . . . . . . . . 




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
    

    // setup database

    if (setupDatabase() != 0){
        perror_die("Database error.");
    }
    #if DEBUG
        printf("%s\n", "Database setup OK.");
    #endif


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


    // creating user "object"
    User* user = (User*) malloc(sizeof(User));

    memset(user->username, '\0', sizeof(user->username));
    memset(user->actual_password, '\0', sizeof(user->actual_password));


    memset(booking.date, '\0', sizeof(booking.date));
    memset(booking.code, '\0', sizeof(booking.code));

    while (1) 
    {
    

        // clean the pipes after receiveing each command.
        memset(command,      '\0', BUFSIZE);
        
    
        

        


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

                rv = usernameIsRegistered(user->username);
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
                #if ENCRYP_PASSWORD 
                    updateUsersRecordFile(user->username, encryptPassword(user->actual_password));
                #else
                    updateUsersRecordFile(user->username, user->actual_password);
                #endif
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
                rv = usernameIsRegistered(command);
                if (rv == 1){
                    state = CHECK_PASSWORD;

                    // storing command (i.e. the username just received)
                    // into the user structure so I can use this in
                    // the next stage to check wheter the password for THIS user 
                    // matches the password previously stored.
                    strcpy(user->username, command);

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

                strcpy(user->actual_password, command);
                printf("password received %s\n", user->actual_password);

                printf("checking if %s %s is in user.txt\n", user->username, user->actual_password);

            
                rv = checkIfPasswordMatches(user->username, user->actual_password);
        
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
                else if (strcmp(command, "res") == 0) // reserve
                    state = CHECK_DATE_VALIDITY;
                else
                    state = LOGIN;
                break;
            
            // update FSM

                break;

            case HELP_LOGGED_IN:
            // do stuff
                writeSocket(conn_sockfd, "H");
            // update FSM
                state = LOGIN;
                break;

            
            case CHECK_DATE_VALIDITY:
                readSocket(conn_sockfd, command); // read date or reserve request

                strcpy(booking.date, command);
                rv = checkDateValidity();
                if (rv == 0){
                    state = CHECK_AVAILABILITY;
                }
                else {
                    state = LOGIN;
                    writeSocket(conn_sockfd, "BADDATE");
                }
                break;

            // check data validity and availability
            case CHECK_AVAILABILITY:
                rv = checkAvailability();
                if (rv == 0){
                    state = RESERVE_CONFIRMATION;
                }
                else {
                    writeSocket(conn_sockfd, "NOAVAL");
                    state = LOGIN;
                }
                break;

            case RESERVE_CONFIRMATION:
                saveReservation(user->username, booking.date, assignRoom(), assignRandomReservationCode());

                writeSocket(conn_sockfd, "RESOK");
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

int commitToDatabase(const char* sql_command){
    sqlite3* db;
    char* err_msg = 0;
    
    int rc = sqlite3_open(DATABASE, &db);
    
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 1;
    }
      
    rc = sqlite3_exec(db, sql_command, 0, 0, &err_msg);
    
    if (rc != SQLITE_OK ) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        
        sqlite3_free(err_msg);        
        sqlite3_close(db);
        
        return 1;
    } 
    
    sqlite3_close(db);
    return 0;
}

int setupDatabase(){

    char *sql_command = QUOTE(
            CREATE TABLE IF NOT EXISTS Bookings(
                `id`        INTEGER     PRIMARY KEY,
                `user`      TEXT        DEFAULT NULL,
                `date`      TEXT        DEFAULT NULL,
                `room`      TEXT        DEFAULT NULL,
                `code`      TEXT        DEFAULT NULL,

                UNIQUE(user, date, room)
            );
    );
    if (commitToDatabase(sql_command) != 0){
        return -1;
    }
    return 0;
}


int usernameIsRegistered(char* u){
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
            return 1;   // found user in the file
        }
    }
    return 0;           // username `u` is new to the system, hence the registration can proceed.
}




int updateUsersRecordFile(char* username, char* encrypted_password){
    
    FILE* users_file = fopen(USER_FILE, "a+");
    if(users_file == NULL) {
        perror_die("fopen()");
    }

    // buffer (will) store the line to be appended to the file.
    char buffer[50];
    memset(buffer, '\0', sizeof(buffer));

    // creating the "payload"
    strcat(buffer, username);
    strcat(buffer, " ");
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

    

int checkIfPasswordMatches(char* username, char* actual_password) {
    char stored_username[30];
    char stored_enc_pass[30];
    char line[50];

    #if ENCRYP_PASSWORD 
        char salt[2];
    #else
    #endif
    

    char res[512]; // result of decryption operation

    FILE* users_file;

    users_file = fopen(USER_FILE, "r");
    if(users_file == NULL) {
        perror("fopen(USER_FILE)");
        return -1;       // no file exists
    }

    

    // Leggo tutte le righe del file
    while(fgets(line, sizeof(line), users_file)){

        // Estraggo username e password dalla riga appena letta
        sscanf(line, "%s %s", stored_username, stored_enc_pass);

        #if ENCRYP_PASSWORD 
            // Estraggo il salt
            salt[0] = stored_enc_pass[0];
            salt[1] = stored_enc_pass[1];

            // Cifro la password inserita dall'utente, uso il salt appena estratto
            strncpy(res, crypt(actual_password, salt), sizeof(res));
        #else
            strcpy(res, actual_password);
        
        #endif
        // username e password cifrata coincidono con quelli del file, l'utente
        // ha eseguito login con successo
        if (strcmp(username, stored_username) == 0 && 
            strcmp(res, stored_enc_pass) == 0)
            return 0;
    }
    return 1;

}


int checkDateValidity(){
    return 0;
}

int checkAvailability(){
    return 0;
}


int saveReservation(char* u, char* d, char* r, char* c){

    /* You may want to add a check to see whether the same 
     * reservation is already stored, even though it's unlikely
     * in a real scenario...
     */

    char *sql_command = malloc(100 * sizeof(char));
    strcat(sql_command, "INSERT or IGNORE INTO Bookings(user, date, room, code) VALUES('");
    strcat(sql_command, u);
    strcat(sql_command, "', '");                           // ^---- databasetable field names
    strcat(sql_command, d);
    strcat(sql_command, "', '");
    strcat(sql_command, r);
    strcat(sql_command, "', '");
    strcat(sql_command, c);
    strcat(sql_command, "');");
    
    #if DEBUG
        printf("%s\n", sql_command);
    #endif
    
    if (commitToDatabase(sql_command) != 0){
        return -1;
    }
    return 0;
}


char* assignRoom(){
    // read from database last room number for that day and 
    // assign that number+1 to the room.
    // if the calculated number exceed the total space of the hotel 
    // notify the client that the operation failed.
    return "1";
}

char* assignRandomReservationCode(){
    static char code[6];
    
    snprintf(
        code, 
        sizeof(code), 
        "%c%c%c%c%c", 
        ((rand()%10)+'A'), ((rand()%10)+'0'),((rand()%10)+'0'), ((rand()%10)+'0'), ((rand()%10)+'A'));
    
    return code;
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

