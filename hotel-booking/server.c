/**
 * @project:        hotel-booking
 * @file:           server.c
 * @author(s):      Francesco Urbani <https://urbanij.github.io/>
 *
 * @date:           Mon Jul  1 12:31:31 CEST 2019
 * @Description:    server side
 *
 *
 * @installation:   On Linux Ubuntu sqlite3 is not shipped by default, install it by typing:
 *                  sudo apt-get install libsqlite3-dev
 *
 *    - optional:   install [DB Browser for SQLite](https://sqlitebrowser.org/)
 *                  (Debian:) sudo apt-get update && sudo apt-get install sqlitebrowser
 *                  (macOS:)  brew cask install db-browser-for-sqlite
 *
 * 
 * @compilation:    `make server` or `gcc server.c -o server [-lcrypt -lpthread] -lsqlite3`
 *
 * LOC: cat server.c | sed '/^\s*$/d' | wc -l
 *
 *
 * TODO:            - checkDateValidity()
 *
 */

/* POSIX libraries */

// standard
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// POSIX threading
#include <pthread.h>
#include "xp_sem.h"

// networking
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>

// security
#ifdef __linux__
    #include <crypt.h>
#else
    // crypt() is part of `unistd.h` on __APPLE__
#endif

// miscellaneous
#include <sqlite3.h>


/* user-defined headers */

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


static char         query_result_g[2048];
static char         rooms_busy_g[4];
static int          entry_id_g;


static int          hotel_total_rooms_g;


/*                       __        __
 *     ____  _________  / /_____  / /___  ______  ___  _____
 *    / __ \/ ___/ __ \/ __/ __ \/ __/ / / / __ \/ _ \/ ___/
 *   / /_/ / /  / /_/ / /_/ /_/ / /_/ /_/ / /_/ /  __(__  )
 *  / .___/_/   \____/\__/\____/\__/\__, / .___/\___/____/
 * /_/                             /____/*/   


/** @brief Thread body for the request handlers. 
 *         Wait for a signal from the main thread, collect the
 *         client file descriptor and serve the request.
 *         Signal the main thread when the request has finished.
 *   @param opaque 
 *   @return Void*
 */
void*   threadHandler           (void* opaque);

/** @brief Command dispatcher: actually serving the requests of the client.
 *         Runs inside the threadHandler functions and dispatches
 *         the inbound commands to the executive functions.
 *
 *  @param sockfd Socket file descriptor
 *  @param thread_index Thread index
 *  @return Void.
 */
void    dispatcher              (int sockfd, int thread_index);

/** @brief Takes a username `u` and opens the file 
 *         `users.txt` to see if such username is
 *         therein contained.
 *  @param u Username
 *  @return 0 if username is in file, -1 otherwise
 */
int     usernameIsRegistered     (char* u);


/** @brief
 *  @param
 *  @param
 *  @return
 */
int     updateUsersRecordFile   (char*, char*);

/** @brief Takes plain text password and return the encrypted version of it
 *  @param password The plain text password
 *  @return encrypted password
 */
char*   encryptPassword         (char* password);

/** @brief
 *  @param
 *  @param
 *  @return
 */
void    makeSalt                (char* salt);


/** @brief
 *  @param
 *  @param
 *  @return
 */
int     checkIfPasswordMatches  (char*, char*);

/** @brief
 *  @param
 *  @param
 *  @return
 */
int     checkDateValidity       ();

/** @brief
 *  @param
 *  @param
 *  @return
 */
int     saveReservation         (char* u, char* d, char* r, char* c);


/** @brief Commit command to database
 *  @param sql_command Sql command to be committed
 *  @return 0 if successful, !0 if not successful.
 */
int     commitToDatabase        (const char* sql_command);

/** @brief Query the database
 *  @param query_id
 *  @param sql_command 
 *  @return struct query (i.e.: return value (int), and query response (char*) )
 */
query_t* queryDatabase          (const int query_id, const char* sql_command);

/** @brief Used by queryDatabase()
 *  @param
 *  @param
 *  @return
 */
int     viewCallback            (void* NotUsed, int argc, char** argv, char** azColName);


/** @brief Used by queryDatabase()
 *  @param
 *  @param
 *  @return
 */
int     busy_roomsCallback      (void* NotUsed, int argc, char** argv, char** azColName);

int     entryValidCallback      (void* NotUsed, int argc, char** argv, char** azColName);

/** @brief Initial database setup. Creates the table Booking.
 *  @return return value (0 OK; !0 not OK)
 */
int     setupDatabase();

/** @brief Assign room to user upon `reserve` request.
 *  @param date
 *  @return room number (string)
 */
char*   assignRoom              (char* date);

/** @brief Generate random reservation code
 *  @return CODE
 */
char*   assignRandomCode        ();

/** @brief Open database and returns the reservation for the user
 *         the called the function.
 *  @param username
 *  @return 
 */
char*   fetchUserReservations   (char* u);

/** @brief 
 *
 *  @param user
 *  @param booking
 *  @return 0 (succ) or -1 (fail)
 */
int     releaseReservation      (User* user, Booking* booking);


// . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .



/********************************/
/*                              */
/*          functions           */
/*                              */
/********************************/



int 
main(int argc, char** argv)
{
    // clear terminal
    system("clear");

    int conn_sockfd;    // connected socket file descriptor

    #ifdef GDB_MODE
        char port[5];
        printf("Insert port number: ");
        scanf("%s", port);

        Address address = (Address){
            .ip   = "127.0.0.1",
            .port = atoi(port)
        };

        hotel_total_rooms_g = 3;
        
    #else
        // reading arguments (IP and port) from stdin
        Address address = readArguments(argc, argv); 

        // reading argument (room number) from stdin
        if (argc < 4){
            printf("\x1b[31mWrong number of parameters!\x1b[0m\n");
            printf("Usage: %s <ip> <port> <hotel rooms>\n", argv[0]);
            exit(-1);
        }
        else {
            hotel_total_rooms_g = atoi(argv[3]);
            if (hotel_total_rooms_g <= 0){
                printf("Usage: %s <ip> <port> <hotel rooms>\n", argv[0]);
                printf("\x1b[31mhotel rooms has to be >= 1\x1b[0m\n");
                exit(-1);
            }
        }

    #endif
    


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

    system("mkdir .data");
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




    while(1) {

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
                                                        "MAIN", // __func__, 
                                                        ip_client, 
                                                        client_addr.sin_port 
                                                    );


        // looking for free thread


    /* critical section */
        xp_sem_wait(&lock_g);

        for (thread_index = 0; thread_index < NUM_THREADS; thread_index++){  //assegnazione del thread
            if (busy[thread_index] == 0) {
                break;
            }
        }

        printf("%s: Thread #%d has been selected.\n",
                                            "MAIN", // __func__, 
                                            thread_index);

        fds[thread_index] = conn_sockfd;    // assigning socket file descriptor to the file descriptors array
        busy[thread_index] = 1;             // notification busy thread, so that it can't be assigned to another, till its release

    /* critical section */
        xp_sem_post(&lock_g);

        
        xp_sem_post(&evsem[thread_index]);        

    }

    // pthread_exit(NULL);



    close(conn_sockfd);


    return 0;
}

/* . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . */



void* 
threadHandler(void* indx)
{
    int thread_index = *(int*) indx;       // unpacking argument
    int conn_sockfd;                       // file desc. local variable

    printf("THREAD #%d ready.\n", thread_index);

    while(1)
    {
        
        // waiting for a request assigned by the main thread
        xp_sem_wait(&evsem[thread_index]);
        printf ("Thread #%d took charge of a request\n", thread_index);

    /* critical section */
        xp_sem_wait(&lock_g);
        conn_sockfd = fds[thread_index];
        xp_sem_post(&lock_g);
    /* end critical section */


        // keep reading messages from client until "quit" arrives.
        
        // serving the request (dispatched)
        dispatcher(conn_sockfd, thread_index);
        printf ("Thread #%d closed session, client disconnected.\n", thread_index);


        // notifying the main thread that THIS thread is now free and can be assigned to new requests.
    /* critical section */
        xp_sem_wait(&lock_g);
        busy[thread_index] = 0;
        xp_sem_post(&lock_g);
    /* end critical section */
        
        xp_sem_post(&free_threads);

    }

    pthread_exit(NULL);
    
}

/* . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . */


void 
dispatcher (int conn_sockfd, int thread_index)
{

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


    // used when processing `view` request and send message back to client.
    // good idea would be to use heap and realloc memory as it grows...
    char reservation_response[BUFSIZE];
    char view_response[BUFSIZE];

    

    while (1) 
    {
    
        // clean the pipes after receiveing each command.
        memset(command, '\0', BUFSIZE);
        
    
        // stores return value, used throughout the loop.
        int rv;


        switch (state)
        {
            case INIT:
                readSocket(conn_sockfd, command);  // fix space separated strings

                if      (strcmp(command, "h") == 0){
                    printf("THREAD #%d: command received: \033[1m%s\x1b[0m\n", thread_index, "help");
                    state = HELP_UNLOGGED;
                }
                else if (strcmp(command, "r") == 0){
                    printf("THREAD #%d: command received: \033[1m%s\x1b[0m\n", thread_index, "register");
                    state = REGISTER;
                }
                else if (strcmp(command, "l") == 0){
                    printf("THREAD #%d: command received: \033[1m%s\x1b[0m\n", thread_index, "login");
                    state = LOGIN_REQUEST;
                }
                else if (strcmp(command, "q") == 0){
                    printf("THREAD #%d: command received: \033[1m%s\x1b[0m\n", thread_index, "quit");
                    state = QUIT;
                }
                else {
                    state = INIT;
                }
                break;
    

            case HELP_UNLOGGED:
                writeSocket(conn_sockfd, "H");
                state = INIT;
                break;



            case REGISTER:
                writeSocket(conn_sockfd, "Choose username: ");

                state = PICK_USERNAME;
                break;

            case PICK_USERNAME:
                readSocket(conn_sockfd, command);
                strcpy(user->username, command);

                printf("usernameee %s\n", user->username);


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
                readSocket(conn_sockfd, command);
                strcpy(user->actual_password, command);

                printf("passworrrd %s\n", user->actual_password);

                state = SAVE_CREDENTIAL;
                break;

            case SAVE_CREDENTIAL:
                // password has to be hashed before storing it.
                #if ENCRYPT_PASSWORD 
                    updateUsersRecordFile(user->username, encryptPassword(user->actual_password));
                #else
                    updateUsersRecordFile(user->username, user->actual_password);
                #endif
                writeSocket(conn_sockfd, "password OK.");
                writeSocket(conn_sockfd, "Successfully registerd, you are now logged-in.");

                state = LOGIN;
                break;

            case LOGIN_REQUEST:
                writeSocket(conn_sockfd, "OK"); // not actually necessary 
                state = CHECK_USERNAME;
                break;

            case CHECK_USERNAME:
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
                
                break;

            case CHECK_PASSWORD:
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
                
                break;

            case GRANT_ACCESS:
                writeSocket(conn_sockfd, "Y");  // Y stands for OK
                state = LOGIN;
                break;

            case LOGIN:
                
                readSocket(conn_sockfd, command);  // fix space separated strings

                if      (strcmp(command, "hh") == 0){ 
                    printf("THREAD #%d: command received: \033[1m%s\x1b[0m\n", thread_index, "help");
                    state = HELP_LOGGED_IN;
                }
                else if (strcmp(command, "q") == 0){  
                    printf("THREAD #%d: command received: \033[1m%s\x1b[0m\n", thread_index, "quit");
                    state = QUIT;
                }
                else if (strcmp(command, "logout") == 0) {
                    printf("THREAD #%d: command received: \033[1m%s\x1b[0m\n", thread_index, "logout");
                    state = INIT;
                }
                else if (strcmp(command, "v") == 0){  
                    printf("THREAD #%d: command received: \033[1m%s\x1b[0m\n", thread_index, "view");
                    state = VIEW;
                }
                else if (strcmp(command, "res") == 0){
                    printf("THREAD #%d: command received: \033[1m%s\x1b[0m\n", thread_index, "reserve");
                    state = CHECK_DATE_VALIDITY;
                }
                else if (strcmp(command, "rel") == 0){
                    printf("THREAD #%d: command received: \033[1m%s\x1b[0m\n", thread_index, "release");
                    state = RELEASE;
                }
                else {
                    state = LOGIN;        
                }

                break;

            case HELP_LOGGED_IN:
                writeSocket(conn_sockfd, "H");
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

                if (strcmp(assignRoom(booking.date), "ERR") != 0){
                    state = RESERVE_CONFIRMATION;
                }
                else {
                    writeSocket(conn_sockfd, "NOAVAL");
                    state = LOGIN;
                }
                break;

            case RESERVE_CONFIRMATION:
                strcpy(booking.room, assignRoom(booking.date));
                strcpy(booking.code, assignRandomCode());

                saveReservation(user->username, booking.date, booking.room, booking.code);

                writeSocket(conn_sockfd, "RESOK");
                writeSocket(conn_sockfd, booking.room);
                writeSocket(conn_sockfd, booking.code);
                state = LOGIN;
                break;

            case VIEW:

                memset(reservation_response, '\0', sizeof(reservation_response));
                memset(query_result_g, '\0', sizeof(query_result_g));
                
                strcpy(reservation_response, fetchUserReservations(user->username));

                
                if (strcmp(reservation_response, "") == 0){
                    writeSocket(conn_sockfd, "You have 0 active reservations.");
                }
                else {
                    memset(view_response, '\0', sizeof(view_response));
                    strcat(view_response, "Your active reservations:\n");
                    strcat(view_response, reservation_response);
                    writeSocket(conn_sockfd, view_response);
                }
                
                state = LOGIN;
                break;

            case RELEASE:

                // init before reading
                memset(&booking, '\0', sizeof booking);

                readSocket(conn_sockfd, booking.date);
                readSocket(conn_sockfd, booking.room);
                readSocket(conn_sockfd, booking.code);

                // force code to be uppercase otherwise does not match in the table.
                upper(booking.code);


                // printf("%s\n", booking.date);
                // printf("%s\n", booking.room);
                // printf("%s\n", booking.code);

                rv = releaseReservation(user, &booking);

                if (rv == 0){
                    writeSocket(conn_sockfd, "\033[92mOK.\x1b[0m Reservation deleted successfully.");
                }
                else {
                    writeSocket(conn_sockfd, "\x1b[31mFailed. \x1b[0mYou have no such reservation.");
                }

                state = LOGIN;

                break;

            case REMOVE_ENTRIES_FROM_DB:
                break;
            
            case QUIT:
                free(state_pointer);
                
                free(user);
    
                
                printf("%s\n", "quitting");     // continue here....
                
                goto ABORT; // yes i know it's bad practice but the label is 
                            // just a few lines below and it's only used here (and in on the client counterpart)

            
            default:
                break;

        }
        
        printServerFSMState(&state, &thread_index);
            
    }

ABORT:
    return;
}


/* . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . */

int 
commitToDatabase(const char* sql_command)
{
    sqlite3* db;
    char* err_msg = 0;
    
    int rc = sqlite3_open(DATABASE, &db);
    
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return -1;
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



query_t* 
queryDatabase(const int query_id, const char* sql_command) 
{
    
    query_t* query = (query_t*) malloc(sizeof(query_t)); // variable to be returned + its initialization
    //<-- MEMSETTING QUERY HERE CAUSED SEVER SEG FAULT where I assign query->id in the swithc case down below.
    #if 0
    memset(query->query_result, '\0', sizeof(query->query_result));
    #endif



    sqlite3* db;
    char* err_msg = 0;
    
    int rc = sqlite3_open(DATABASE, &db);

    
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        
        query->rv = -1;
        query->query_result = (void*)"";
        return query;
    }
    


    switch (query_id){
        case 0:
            rc = sqlite3_exec(db, sql_command, viewCallback, 0, &err_msg);
            break;
        
        case 1:
            rc = sqlite3_exec(db, sql_command, busy_roomsCallback, 0, &err_msg);  
            break;

        case 2:
            rc = sqlite3_exec(db, sql_command, entryValidCallback, 0, &err_msg); 
            break;
    }

    #if DEBUG
        printf("Querying: %s\n", sql_command);
    #endif




    if (rc != SQLITE_OK ) {
        fprintf(stderr, "Failed to select data\n");
        fprintf(stderr, "SQL error: %s\n", err_msg);

        sqlite3_free(err_msg);
        sqlite3_close(db);
        
        query->rv = -1;
        query->query_result = (void*)"";
        return query;
    } 

    sqlite3_close(db);


    // based on the query return the appropriate variable
    

    switch (query_id){
        case 0:

            // removing new line from last entry in list of entries (`view` response)
            query_result_g[strlen(query_result_g)-1] = '\0';

            query->rv = 0;
            query->query_result = (void*) query_result_g;
            break;
        
        case 1:
            query->rv = 0;
            query->query_result = (void*) rooms_busy_g;
            break;
        case 2:
            query->rv = 0;
            query->query_result = (void*) &entry_id_g;
            break;
    }

    // printf("ddd %d\n", *(int*)(query->query_result));


    return query;
}


int 
viewCallback (void* NotUsed, int argc, char** argv, char** azColName) 
{
    /*  Format of returned string:
     *  18/06/2020, room 21, reserve code: 8GT4A
     */


    // query_result_g is the payload "to be returned". 
    // query_result_g is a global variable!

    // clean payload right before filling if from scratch.
    // memset(query_result_g, '\0', sizeof(query_result_g));

    #if 0

        char tmp_str[40] = "";

        snprintf(tmp_str, sizeof(tmp_str), "%s/2020, room %s, reserve code: %s", argv[2], argv[3], argv[4]);
        
        strcat(query_result_g, tmp_str);

    #else
        
        char tmp_str[64];
        memset(tmp_str, '\0', sizeof(tmp_str));


        for (int i = 0; i < argc; i++)
        {
            if (i==0){
                snprintf(tmp_str, sizeof(tmp_str), "   %s/2020, room ", argv[i]);
            }
            else if (i == 1){
                snprintf(tmp_str, sizeof(tmp_str), "%s, reserve code: ", argv[i]);   
            }
            else if (i == 2){
                snprintf(tmp_str, sizeof(tmp_str), "%s", argv[i]);   
            }
            strcat(query_result_g, tmp_str);

        }
        
        strcat(query_result_g, "\n");
        
    #endif

    return 0;
}




int 
busy_roomsCallback(void* NotUsed, int argc, char** argv, char** azColName) 
{

    

    // rooms_busy_g is the payload "to be returned". 
    // rooms_busy_g is a global variable!

    // clean payload right before filling if from scratch.
    memset(rooms_busy_g, '\0', sizeof(rooms_busy_g));
    
    strcpy(rooms_busy_g, argv[0]);  // the variable to be returned is
                                    // only a number (str) and only a single argument, hence argv[0]
    return 0;
}

int 
entryValidCallback(void* NotUsed, int argc, char** argv, char** azColName) 
{   
    entry_id_g = atoi(argv[0]);

    #if DEBUG
        printf("Releasing entry with id %d\n", entry_id_g);
    #endif
    
    return 0;
}



int 
setupDatabase()
{

    char* sql_command = QUOTE(
            CREATE TABLE IF NOT EXISTS Bookings(
                `id`        INTEGER     PRIMARY KEY,
                `user`      TEXT        DEFAULT NULL,
                `date`      TEXT        DEFAULT NULL,
                `room`      TEXT        DEFAULT NULL,
                `code`      TEXT        DEFAULT NULL,

                UNIQUE(user, date, code)
            );
    );

    int rv;
    rv = commitToDatabase(sql_command);
    
    return rv;  // 0 meaning ok, -1 not ok
}


int 
usernameIsRegistered(char* u)
{
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



int 
updateUsersRecordFile(char* username, char* encrypted_password)
{
    
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

char* 
encryptPassword(char* password)
{
    // encrypted password returned 
    // (static because it has to outlive the time-scope of the function)
    static char res[512];   
    
    char salt[2];           // salt

    makeSalt(salt);         // making salt

    // copy encrypted password to res and return it.
    strncpy(res, crypt(password, salt), sizeof(res)); 
    //             ^--- CRYPT(3)    BSD Library Functions Manual

    return res;
}


void 
makeSalt(char* salt) 
{
    int r;  // temp variable

    // init random number generator
    srand(time(NULL));
    
    // make sure salt chars are actual characters, not than special ASCII symbols
    r = rand();
    r = r % ('z' - '0');
    salt[0] = '0' + r;
    
    r = rand();
    r = r % ('z' - '0');
    salt[1] = '0' + r;
    
    #if 0
        printf("Salt: %c%c\n", salt[0], salt[1]);
    #endif
}

    

int 
checkIfPasswordMatches(char* username, char* actual_password) 
{
    char stored_username[30];
    char stored_enc_pass[30];
    char line[50];

    #if ENCRYPT_PASSWORD 
        char salt[2];
    #else
    #endif
    

    char res[512]; // result of decryption operation

    FILE* users_file;

    users_file = fopen(USER_FILE, "r");
    if(users_file == NULL) {
        perror("fopen(USER_FILE)");
        return -1;       // file is missing...
    }


    // Scroll through the lines of users_file
    while(fgets(line, sizeof(line), users_file)){

        // get username and password from line
        sscanf(line, "%s %s", stored_username, stored_enc_pass);

        #if ENCRYPT_PASSWORD 
            // retrieve salt
            salt[0] = stored_enc_pass[0];
            salt[1] = stored_enc_pass[1];

            // encrypt the password given by the user with THAT salt
            strncpy(res, crypt(actual_password, salt), sizeof(res));
        #else
            strcpy(res, actual_password);
        
        #endif
        // if username and password match, login is successful.
        if (strcmp(username, stored_username) == 0 && 
            strcmp(res, stored_enc_pass) == 0)
            return 0;
    }
    return 1;

}


int 
checkDateValidity()
{
    return 0;
}


int 
saveReservation(char* u, char* d, char* r, char* c)
{

    /* You may want to add a check to see whether the same 
     * reservation is already stored, even though it's unlikely
     * in a real scenario...
     */

    int rv;

    char sql_command[256 * sizeof(char)];

    memset(sql_command, '\0', sizeof(sql_command));

    strcat(sql_command, "INSERT or IGNORE INTO Bookings(user, date, room, code) VALUES('");
    strcat(sql_command, u);
    strcat(sql_command, "', '");                    //         ^---- database table field names
    strcat(sql_command, d);
    strcat(sql_command, "', '");
    strcat(sql_command, r);
    strcat(sql_command, "', '");
    strcat(sql_command, c);
    strcat(sql_command, "');");
    
    #if DEBUG
        printf("%s\n", sql_command);
    #endif
    
    rv = commitToDatabase(sql_command);

    if (rv != 0){
        return -1;
    }
    return 0;
}


char* 
assignRoom(char* date)
{
    /* read from database last room number for that day and 
     * assign that number+1 to the room.
     * if the calculated number exceed the total space of the hotel 
     * notify the client that the operation failed.
        
        fix this. 
        if room get released if another room is booked the room number is duplicated.
    
     */

    
    char sql_command[1024];
    memset(sql_command, '\0', sizeof(sql_command));

    strcat(sql_command, "SELECT COUNT(room) FROM Bookings WHERE date = '");
    strcat(sql_command, date);
    strcat(sql_command, "'");



    // Querying the database with the command just created
    query_t* query = (query_t*) malloc(sizeof(query_t));
    // memset(&query, 0, sizeof query );

    query = queryDatabase(1, sql_command);


    // Checking the results of the query
    if (query->rv == 0){     // if return value is not 0, something went wrong.

        int rooms_busy;

        // turn into integer the result of the query 
        // i.e.: rooms busy for the specified date.
        rooms_busy = atoi(query->query_result);
        rooms_busy++;

        if (rooms_busy > hotel_total_rooms_g){
            free(query);
            return "ERR";
        }
        else {
            
            static char rooms_busy_str[4];
            sprintf(rooms_busy_str, "%d", rooms_busy);

            free(query);
            return rooms_busy_str;

        }
    }
    else {
        printf("%s\n", "Error querying the database!");
        free(query);
        return "";
    }

}


char* 
assignRandomCode()
{

    // init seed
    srand(time(NULL));

    // rand code to be returned
    static char code[6];

    // Not only randomize code but also "randomize" (3 options) position of letter and digit.
    int randomnumber;
    randomnumber = rand() % 3;

    switch (randomnumber){
        case 0:
        default:
            snprintf(
                code, 
                sizeof(code), 
                "%c%c%c%c%c", 
                ((rand()%10)+'A'), ((rand()%10)+'0'),((rand()%10)+'0'), ((rand()%10)+'0'), ((rand()%10)+'A')
            );
            break;


        case 1:
            snprintf(
                code, 
                sizeof(code), 
                "%c%c%c%c%c", 
                ((rand()%10)+'A'), ((rand()%10)+'A'),((rand()%10)+'0'), ((rand()%10)+'0'), ((rand()%10)+'A')
            );
            break;

        case 2:
            snprintf(
                code, 
                sizeof(code), 
                "%c%c%c%c%c", 
                ((rand()%10)+'A'), ((rand()%10)+'0'),((rand()%10)+'0'), ((rand()%10)+'A'), ((rand()%10)+'A')
            );
            break;
    }

    return code;
}



char* 
fetchUserReservations(char* u)
{
    query_t* query = (query_t*) malloc(sizeof(query_t));
 
    char sql_command[1024];
    memset(sql_command, '\0', sizeof(sql_command));

    strcat(sql_command, "SELECT date, room, code FROM Bookings WHERE user = '");
    strcat(sql_command, u);
    strcat(sql_command, "' ORDER BY id"); // i.e. order by reservation basically
    

    query = queryDatabase(0, sql_command);


    if (query->rv == 0){
        // free(query); // cant free here because it has to be returned... ¯\_(ツ)_/¯
        return (char*) query->query_result;
    }
    else {
        printf("%s\n", "Error querying the database!");

        free(query);
        return "";
    }

}


int 
releaseReservation(User* user, Booking* booking)
{

    /* check if entry is found in database
     * if YES: delete it
     * if NO: return failure value to main function
     */

    // preparing payload
    char sql_command[1024];
    memset(sql_command, '\0', sizeof(sql_command));
    strcat(sql_command, "SELECT COUNT(id) FROM Bookings WHERE user = '");
    strcat(sql_command, user->username);
    strcat(sql_command, "' and date = '");
    strcat(sql_command, booking->date);
    strcat(sql_command, "' and room = '");
    strcat(sql_command, booking->room);
    strcat(sql_command, "' and code = '");
    strcat(sql_command, booking->code);
    strcat(sql_command, "'");
    

    // allocating dynamic variable where the result of the query will be stored.
    query_t* query = (query_t*) malloc(sizeof(query_t));

    // memset(query->query_result, '\0', sizeof(query->query_result));     // FIX this

                    // debug at its finest here                        
                    // printf("%lu\n", sizeof(query_t));
                    // printf("%lu\n", sizeof(int));
                    // printf("%lu\n", sizeof(void*));
                    // printf("%lu\n", sizeof(char*));

    query = queryDatabase(2, sql_command);
    
    if (query->rv == 0){
        
        if ( *((int*) query->query_result) == 0){  //1 if in database, 0 otherwise.
            
            free(query);
            return -1;
        }

        else {

            // prepare payload for wiping the entry from the table

            memset(sql_command, '\0', sizeof(sql_command));
            strcat(sql_command, "DELETE FROM Bookings WHERE user = '");
            strcat(sql_command, user->username);
            strcat(sql_command, "' and date = '");
            strcat(sql_command, booking->date);
            strcat(sql_command, "' and room = '");
            strcat(sql_command, booking->room);
            strcat(sql_command, "' and code = '");
            strcat(sql_command, booking->code);
            strcat(sql_command, "'");

            #if DEBUG
                printf("Deleting from DB: %s\n", sql_command);
            #endif

            // freeing memory before returning
            free(query);
            return commitToDatabase(sql_command); // 0 is ok.

        }
    
    }
    else {
        printf("%s\n", "Error querying the database!");

        free(query);
        return -1;
    }

}





