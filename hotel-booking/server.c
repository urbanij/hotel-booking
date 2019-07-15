/**
 * @name            hotel-booking
 * @file            server.c
 * @author          Francesco Urbani <https://urbanij.github.io/>
 *
 * @date            Mon Jul  1 12:31:31 CEST 2019
 * @brief           server side
 *
 *
 * *installation    On Linux Ubuntu sqlite3 is not shipped by default, install it by typing:
 *                  sudo apt-get install libsqlite3-dev
 *
 *    **optional    install [DB Browser for SQLite](https://sqlitebrowser.org/)
 *                  (Debian:) sudo apt-get update && sudo apt-get install sqlitebrowser
 *                  (macOS:)  brew cask install db-browser-for-sqlite
 *
 * 
 * *compilation     `make server` or `gcc server.c -o server [-lcrypt -lpthread] -lsqlite3`
 *
 *
 */

// standard
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// POSIX threading
#include <pthread.h>    // gcc requires -lpthread flag 
#include "xp_sem.h"

// networking
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>

// security
#ifdef __linux__
    #include <crypt.h>  // gcc requires -crypt flag 
    // crypt() is part of `unistd.h` on __APPLE__
#endif

// miscellaneous
#include <sqlite3.h>    // gcc requires -lsqlite3 flag 


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

/* . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . */

/********************************/
/*                              */
/*            macros            */
/*                              */
/********************************/

#define QUOTE(...)          #__VA_ARGS__     
        /* this define allowes me to avoid writing every 
         * SQL command within quotes.
         * https://stackoverflow.com/a/17996915/6164816 
         */


#define SQLITE_THREADSAFE   1   // 1 is the default 

        /* from documentation: https://www.sqlite.org/threadsafe.html
         *  Single-thread   : 0     : all mutexes are disabled and SQLite is unsafe to use 
         *                            in more than a single thread at once.
         *  Serialized      : 1     : SQLite can be safely used by multiple threads with no restriction.
         *  Multi-thread    : 2     : SQLite can be safely used by multiple threads provided that no 
         *                            single database connection is used simultaneously in two or more threads
         *
         * Setting SQL mode to Multi-thread. 
         * In this mode, SQLite can be safely used by multiple threads provided 
         * that no single database connection is used simultaneously in two or more threads.
         */

/* . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . */

/********************************/
/*                              */
/*       global variables       */
/*                              */
/********************************/

static pthread_mutex_t  lock_g;                     // global lock
static pthread_mutex_t  users_lock_g;               // global lock for accessing the file `users.txt`


static xp_sem_t         free_threads;               // semaphore for waiting for free threads
static xp_sem_t         evsem[NUM_THREADS];         // per-thread event semaphores

static int              fds[NUM_THREADS];           // array of file descriptors


static int              busy[NUM_THREADS];          // map of busy threads
static int              tid[NUM_THREADS];           // array of pre-allocated thread IDs
static pthread_t        threads[NUM_THREADS];       // array of pre-allocated threads



static char             query_result_g[BUFSIZE];    // response of `view` query.            Used in `viewCallback()`
static char             rooms_busy_g[4];            // response of `SELECT COUNT...` query. Used in `busy_roomsCallback()`
static int              entry_id_g;                 // Used in `validEntryCallback()`. Stores whether entry is in Bookings table or not.


static int              hotel_max_available_rooms;  // hotel max available rooms. Read from stdin as soon as the program starts.



                                                    // folder path + file name saved in `config.h` merge
static char             USER_FILE[30];              // user file path
static char             DATABASE[30];               // database  path 



/* . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . */

/********************************/
/*                              */
/*      functions prototype     */
/*                              */
/********************************/


/** @brief Thread body for the request handlers. 
 *         Wait for a signal from the main thread, collect the
 *         client file descriptor and serve the request.
 *         Signal the main thread when the request has finished.
 *   @param opaque 
 *   @return Void*
 */
void*       threadHandler(void* opaque);

/** @brief Command dispatcher: actually serving the requests of the client.
 *         Runs inside the threadHandler functions and dispatches
 *         the inbound commands to the executive functions.
 *
 *  @param sockfd Socket file descriptor
 *  @param thread_index Thread index
 *  @return Void.
 */
void        dispatcher(int sockfd, int thread_index);

/** @brief Takes a username `u` and opens the file 
 *         `users.txt` to see if such username is
 *         therein contained.
 *  @param u Username
 *  @return 0 if username is in file, -1 otherwise
 */
int         usernameIsRegistered(char* u);


/** @brief  Opens the file where the users are stored
 *          and update it with the new data (parameter of fucntion)
 *  @param  username new username to be added
 *  @param  password new password to be added
 *  @return 0 if ok; dies otherwise.
 */
int         updateUsersRecordFile(char* username, char* password);

/** @brief  Takes plain text password and return the encrypted version of it
 *  @param thread index used from printing purposes
 *  @param password The plain text password
 *  @return encrypted password
 */
char*       encryptPassword(int thread_index, char* password);


/** @brief   `Opens users.txt` and checks whether password of user `user` matches.
 *  @param    user
 *  @return   0 : ok. 
 *            -1: failure. 
 *            1 : does not match.
 */
int         checkIfPasswordMatches(User* user);


/** @brief  save user reservation to database
 *  @param thread index used from printing purposes
 *  @param user
 *  @param booking booking data
 *  @return 
 */
int         saveReservation(int thread_index, User* user, Booking* booking);


/** @brief Commit command to database
 *  @param sql_command Sql command to be committed
 *  @param thread index used from printing purposes
 *  @return 0 if successful, !0 if not successful.
 */
int         commitToDatabase(int thread_index, const char* sql_command);

/** @brief Query the database
 *  @param thread index used from printing purposes
 *  @param query_id
 *  @param sql_command 
 *  @return struct query (i.e.: return value (int), and query response (char*) )
 */
query_t*    queryDatabase(int thread_index, const int query_id, const char* sql_command);

/** @brief Used by queryDatabase()
 *  @param
 *  @param
 *  @return
 */
int         viewCallback(void* NotUsed, int argc, char** argv, char** azColName);


/** @brief Used by queryDatabase()
 *  @param
 *  @param
 *  @return
 */
int         busy_roomsCallback(void* NotUsed, int argc, char** argv, char** azColName);


/** @brief Used by queryDatabase()
 *  @param
 *  @param
 *  @return
 */
int         validEntryCallback(void* NotUsed, int argc, char** argv, char** azColName);

/** @brief Initial database setup. Creates the table Booking.
 *  @return return value (0 OK; !0 not OK)
 */
int         setupDatabase();

/** @brief Assign room to user upon `reserve` request.
 *  @param thread index used from printing purposes
 *  @param date
 *  @return room number (string) or "ERR" to notify error (no room available)
 */
char*       assignRoom(int thread_index, char* date);

/** @brief Generate random string. Used both for CODE generatio and for salt generation
 *  @param str the random string generated
 *  @param size the length of the random string to be generated
 *  @return Void
 */
void        generateRandomString(char* str, size_t size);

/** @brief Open database and returns the reservation for the user
 *         the called the function.
 *  @param thread index used from printing purposes
 *  @param username
 *  @return 
 */
char*       fetchUserReservations(int thread_index, User* user);

/** @brief   release reservation and wipe related entry from databse.
 *  @param user
 *  @param booking
 *  @return 0 (succ) or -1 (fail).
 */
int         releaseReservation(int thread_index, User* user, Booking* booking);


/* . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . */


int 
main(int argc, char** argv)
{
    // clear terminal
    system("clear");

    
    //
    strcat(USER_FILE, DATA_FOLDER);
    strcat(USER_FILE, "/");
    strcat(USER_FILE, USER_FILE_NAME);

    strcat(DATABASE, DATA_FOLDER);
    strcat(DATABASE, "/");
    strcat(DATABASE, DATABASE_NAME);



    int conn_sockfd;    // connected socket file descriptor


    #if GDB_MODE
        char port[5];
        printf("Insert port number: ");
        scanf("%s", port);

        Address address = (Address){
            .ip   = "127.0.0.1",
            .port = atoi(port)
        };

        hotel_max_available_rooms = 3;
        
    #else
        // reading from std in. readArguments is used soleley for IP and port since it's mutual with the client.
        // room number is read later

        // reading arguments (IP and port) from stdin
        Address address = readArguments(argc, argv); 

        // reading argument (room number) from stdin
        if (argc < 4){
            printf("\x1b[31mWrong number of parameters!\x1b[0m\n");
            printf("Usage: %s <ip> <port> <hotel rooms>\n", argv[0]);
            exit(-1);
        }
        else {
            hotel_max_available_rooms = atoi(argv[3]);
            if (hotel_max_available_rooms <= 0){
                printf("Usage: %s <ip> <port> <hotel rooms>\n", argv[0]);
                printf("\x1b[31mhotel rooms has to be >= 1\x1b[0m\n");
                exit(-1);
            }
        }

    #endif
    


    // setup the server and return socket file descriptor.
    int sockfd = setupServer(&address);     // listening socket file descriptor


    // setup semaphores
    pthread_mutex_init(&lock_g, 0);
    pthread_mutex_init(&users_lock_g, 0);
    xp_sem_init(&free_threads, 0, NUM_THREADS);


    char ip_client[INET_ADDRSTRLEN];
    

    // setup database
    char mkdir_command[7 + sizeof(DATA_FOLDER)] = "mkdir ";
    strcat(mkdir_command, DATA_FOLDER); // DATA_FOLDER set inside `config.h`
    system(mkdir_command);

    if (setupDatabase() != 0){
        perror_die("Database error.");
    }
    #if DEBUG
        printf(ANSI_COLOR_GREEN "[+] Database setup OK.\n" ANSI_COLOR_RESET );
    #endif




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
                                                        "MAIN", // __func__, 
                                                        ip_client, 
                                                        address.port // client_addr.sin_port 
                                                    );


        // looking for free thread

        /* critical section */
        pthread_mutex_lock(&lock_g);

            for (thread_index = 0; thread_index < NUM_THREADS; thread_index++){  
                // thread assignment

                if (busy[thread_index] == 0) {
                    break;
                }
            }

            printf("%s: Thread #%d has been selected.\n",
                                                "MAIN", // __func__, 
                                                thread_index);

            fds[thread_index] = conn_sockfd;    // assigning socket file descriptor to the file descriptors array
            busy[thread_index] = 1;             // notification busy thread, so that it can't be assigned to another, till its release

        /* end critical section */
        pthread_mutex_unlock(&lock_g);

        
        xp_sem_post(&evsem[thread_index]);        

    }

    // pthread_exit(NULL);



    close(conn_sockfd);


    return 0;

} // end main

/* . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . */

/********************************/
/*                              */
/*          functions           */
/*                              */
/********************************/



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
        
        
        /* critical section */
        pthread_mutex_lock(&lock_g);
            conn_sockfd = fds[thread_index];
        pthread_mutex_unlock(&lock_g);
        /* end critical section */


        // keep reading messages from client until "quit" arrives.
        
        // serving the request (dispatched)
        dispatcher(conn_sockfd, thread_index);
        
        printf ("Thread #%d closed session, client disconnected.\n", thread_index);
    


    
        // notifying the main thread that THIS thread is now free and can be assigned to new requests.
    
        /* critical section */
        pthread_mutex_lock(&lock_g);
            busy[thread_index] = 0;
            
            printf("MAIN: Thread #%d has been freed.\n", thread_index);
        
        pthread_mutex_unlock(&lock_g);
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



    // creating variable for holding FSM current state on server side.
    server_fsm_state_t state = INIT;

    

    // creating user "object"
    User* user = (User*) malloc(sizeof(User));

    memset(user->username, '\0', sizeof(user->username));
    memset(user->actual_password, '\0', sizeof(user->actual_password));


    memset(booking.date, '\0', sizeof(booking.date));
    memset(booking.code, '\0', sizeof(booking.code));


    // used when processing `view` request and send message back to client.
    // ! good idea would be to use heap and realloc memory as it grows.
    char reservation_response[BUFSIZE];
    char view_response[BUFSIZE];

    

    while (1) 
    {
    
    
        // stores return value, used throughout the loop.
        int rv;


        switch (state)
        {
            case INIT:
                memset(command, '\0', BUFSIZE);
                readSocket(conn_sockfd, command);  // fix space separated strings

                if      (strcmp(command, HELP_MSG) == 0){
                    printf("THREAD #%d: command received: \033[1m%s\x1b[0m\n", thread_index, "help");
                    state = HELP_UNLOGGED;
                }
                else if (strcmp(command, REGISTER_MSG) == 0){
                    printf("THREAD #%d: command received: \033[1m%s\x1b[0m\n", thread_index, "register");
                    state = REGISTER;
                }
                else if (strcmp(command, LOGIN_MSG) == 0){
                    printf("THREAD #%d: command received: \033[1m%s\x1b[0m\n", thread_index, "login");
                    state = LOGIN_REQUEST;
                }
                else if (strcmp(command, QUIT_MSG) == 0){
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
                memset(command, '\0', BUFSIZE);
                readSocket(conn_sockfd, command);
                strcpy(user->username, command);

                #if VERBOSE_DEBUG
                    printf("Username inserted: \033[1m%s\x1b[0m\n", user->username);
                #endif

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
                memset(command, '\0', BUFSIZE);
                readSocket(conn_sockfd, command);
                strcpy(user->actual_password, command);

                #if VERBOSE_DEBUG
                    printf("Thread #%d: Plain text password inserted: \033[1m%s\x1b[0m\n", thread_index, user->actual_password);
                #endif

                state = SAVE_CREDENTIAL;
                break;

            case SAVE_CREDENTIAL:
                
                #if ENCRYPT_PASSWORD 
                    updateUsersRecordFile(user->username, encryptPassword(thread_index, user->actual_password));
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
                memset(command, '\0', BUFSIZE);
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
                memset(command, '\0', BUFSIZE);
                readSocket(conn_sockfd, command);

                strcpy(user->actual_password, command);

                #if VERBOSE_DEBUG
                    printf("Thread #%d: Plain text password received \033[1m%s\x1b[0m\n", thread_index, user->actual_password);
                #endif

                #if DEBUG
                    printf("Thread #%d: checking whether \033[1m%s\x1b[0m is in user.txt\n", thread_index, user->username);
                #endif


                rv = checkIfPasswordMatches(user);


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
                
                memset(command, '\0', BUFSIZE);
                readSocket(conn_sockfd, command);  // fix space separated strings

                if      (strcmp(command, HELP_MSG) == 0){ 
                    printf("THREAD #%d: command received: \033[1m%s\x1b[0m\n", thread_index, "help");
                    state = HELP_LOGGED_IN;
                }
                else if (strcmp(command, QUIT_MSG) == 0){  
                    printf("THREAD #%d: command received: \033[1m%s\x1b[0m\n", thread_index, "quit");
                    state = QUIT;
                }
                else if (strcmp(command, LOGOUT_MSG) == 0) {
                    printf("THREAD #%d: command received: \033[1m%s\x1b[0m\n", thread_index, "logout");
                    state = INIT;
                }
                else if (strcmp(command, VIEW_MSG) == 0){  
                    printf("THREAD #%d: command received: \033[1m%s\x1b[0m\n", thread_index, "view");
                    state = VIEW;
                }
                else if (strcmp(command, RESERVE_MSG) == 0){
                    printf("THREAD #%d: command received: \033[1m%s\x1b[0m\n", thread_index, "reserve");
                    state = CHECK_DATE_VALIDITY;
                }
                else if (strcmp(command, RELEASE_MSG) == 0){
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
                memset(command, '\0', BUFSIZE);
                readSocket(conn_sockfd, command); // read date or reserve request


                // ! useless state: check date validity is performed on client side. 
                strcpy(booking.date, command);
                rv = 0; // formerly   rv = checkDateValidity();
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

                if (strcmp(assignRoom(thread_index, booking.date), "FULL") != 0){
                    state = RESERVE_CONFIRMATION;
                }
                else {
                    writeSocket(conn_sockfd, "NOAVAL");
                    state = LOGIN;
                }
                break;

            case RESERVE_CONFIRMATION:
                strcpy(booking.room, assignRoom(thread_index, booking.date));
                
                // strcpy(booking.code, generateRandomString());
                generateRandomString(booking.code, RESERVATION_CODE_LENGTH);
                // make sure the code is all uppercase
                upper(booking.code);


                saveReservation(thread_index, user, &booking);

                writeSocket(conn_sockfd, "RESOK");
                writeSocket(conn_sockfd, booking.room);
                writeSocket(conn_sockfd, booking.code);
                state = LOGIN;
                break;

            case VIEW:

                memset(reservation_response, '\0', sizeof(reservation_response));
                memset(query_result_g, '\0', sizeof(query_result_g));
                
                strcpy(reservation_response, fetchUserReservations(thread_index, user));


                if (strcmp(reservation_response, "") == 0){
                    writeSocket(conn_sockfd, "You have 0 active reservations.");
                }
                else {

                    // can be moved to server side except reservation_response...
                    memset(view_response, '\0', sizeof(view_response));
                    strcat(view_response, "Your active reservations in 2020");
                    #if SORT_VIEW_BY_DATE
                        strcat(view_response, " sorted by DATE");
                    #else

                    #endif
                    strcat(view_response, ":\n");
                    strcat(view_response, "+------+------+------+\n");
                    strcat(view_response, "| date | room | code |\n");
                    strcat(view_response, "+------+------+------+\n");
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


                rv = releaseReservation(thread_index, user, &booking);

                if (rv == 0){
                    writeSocket(conn_sockfd, "\033[92mOK.\x1b[0m Reservation deleted successfully.");
                }
                else {
                    writeSocket(conn_sockfd, "\x1b[31mFailed. \x1b[0mYou have no such reservation.");
                }

                state = LOGIN;

                break;

            case QUIT:
                strcpy(command, "abort");
                
                free(user);
    
                printf("THREAD #%d: quitting\n", thread_index);
                break;

        }

        
        printServerFSMState(&state, &thread_index);
        

        // check whether the only coomand that would the program exit the while loop has arrived.
        if (strcmp(command, "abort") == 0){
            return;
        }

    
    }

}


/* . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . */

int 
commitToDatabase(int thread_index, const char* sql_command)
{
    sqlite3* db;
    char* err_msg = 0;
    
    int rc = sqlite3_open(DATABASE, &db);
    
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return -1;
    }
    

    #if VERBOSE_DEBUG
        printf("Thread #%d: Committing to database:\n   %s\n", thread_index, sql_command);
    #endif


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
queryDatabase(int thread_index, const int query_id, const char* sql_command) 
{
    
    query_t* query = (query_t*) malloc(sizeof(query_t)); // variable to be returned + its initialization


    sqlite3* db;
    char* err_msg = 0;
    
    int rc = sqlite3_open(DATABASE, &db);

    
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database:\n%s\n", sqlite3_errmsg(db));
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
            rc = sqlite3_exec(db, sql_command, validEntryCallback, 0, &err_msg); 
            break;
    }

    #if VERBOSE_DEBUG
        printf("Thread #%d: Querying from database:\n   %s\n", thread_index, sql_command);
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

    
    char tmp_str[64];
    memset(tmp_str, '\0', sizeof(tmp_str));


    for (int i = 0; i < argc; i++)
    {
        if (i==0){
            snprintf(tmp_str, sizeof(tmp_str), "  %s   ", argv[i]);
        }
        else if (i == 1){
            snprintf(tmp_str, sizeof(tmp_str), "%s     ", argv[i]);   
        }
        else if (i == 2){
            snprintf(tmp_str, sizeof(tmp_str), "%s", argv[i]);   
        }
        strcat(query_result_g, tmp_str);

    }
    
    strcat(query_result_g, "\n");
        

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
validEntryCallback(void* NotUsed, int argc, char** argv, char** azColName) 
{   
    entry_id_g = atoi(argv[0]);

    #if VERY_VERBOSE_DEBUG
        printf("Releasing entry with id %d\n", entry_id_g);
    #endif
    
    return 0;
}



int 
setupDatabase()
{

    char* sql_command = QUOTE(
            CREATE TABLE IF NOT EXISTS Bookings(
                `id`            INTEGER     PRIMARY KEY,
                `user`          TEXT        DEFAULT NULL,
                `date`          TEXT        DEFAULT NULL,
                `date_yyyymmdd` TEXT        DEFAULT NULL,
                `room`          TEXT        DEFAULT NULL,
                `code`          TEXT        DEFAULT NULL,

                UNIQUE(user, date, room)
            );
    );

    int rv;
    rv = commitToDatabase(-1, sql_command);
    
    return rv;  // 0 meaning ok, -1 not ok
}


int 
usernameIsRegistered(char* u)
{
    char username[30];
    char enc_pass[30];
    char line[50];

    // locking shared resource
    pthread_mutex_lock(&users_lock_g);

    FILE* users_file;

    users_file = fopen(USER_FILE, "r");
    if(users_file == NULL) {
        pthread_mutex_unlock(&users_lock_g);
        return 0;       // no file exists, hence no username exists, gr8
    }

    // check whether the user is already in the file (= already registered)
    while(fgets(line, sizeof(line), users_file)) {
        sscanf(line, "%s %s\n", username, enc_pass);
        
        // checking whether `u` (i.e. username coming from the client) 
        // matches `username` (i.e. username saved on the file in the server)
        if (strcmp(u, username) == 0){
            fclose(users_file);
            
            // unlocking shared resource before going returning to caller
            pthread_mutex_unlock(&users_lock_g);
            return 1;   // found user in the file
        }
    }

    fclose(users_file);

    // unlocking shared resource before going returning to caller
    pthread_mutex_unlock(&users_lock_g);
    
    return 0;           // username `u` is new to the system, hence the registration can proceed.
}



int 
updateUsersRecordFile(char* username, char* encrypted_password)
{
    // buffer (will) store the line to be appended to the file.
    char buffer[USERNAME_MAX_LENGTH + PASSWORD_MAX_LENGTH + 2];
    memset(buffer, '\0', sizeof(buffer));

    // creating the "payload"
    strcat(buffer, username);
    strcat(buffer, " ");
    strcat(buffer, encrypted_password);

    // protecting shared resource access with semaphore
    pthread_mutex_lock(&users_lock_g);

    FILE* users_file;
    users_file = fopen(USER_FILE, "a+");
    if (users_file == NULL) {
        perror_die("fopen()");
    }

    // add line
    fprintf(users_file, "%s\n", buffer);

    // close connection to file
    fclose(users_file);

    pthread_mutex_unlock(&users_lock_g);

    return 0;
}

char* 
encryptPassword(int thread_index, char* password)
{
    // encrypted password returned 
    // (static because it has to outlive the time-scope of the function)
    static char res[512];   
    
    char salt[2];           // salt



    /* generating salt here:
     * the function makeSalt caused a random seg fault to occur:
     * when a non-alphabetic character is generated such as '<', 
     * the function crypt does not know how to handle it and 
     * random crash occurs.
     * The way i generate the salt instead is by using generateRandomString function
     * which I would have written regardless for generating the random reservation code.
     */
    
    generateRandomString(salt, 3);  // 3:   2 for salt, 1 for '\0'.
    
    #if VERBOSE_DEBUG
        printf("Thread #%d: Salt: %c%c\n", thread_index, salt[0], salt[1]);
    #endif

    

    // copy encrypted password to res and return it.
    strncpy(res, crypt(password, salt), sizeof(res)); 
    //             ^--- CRYPT(3)    BSD Library Functions Manual

    return res;
}
    

int 
checkIfPasswordMatches(User* user) 
{
    char stored_username[USERNAME_MAX_LENGTH];
    char stored_enc_psswd[PASSWORD_MAX_LENGTH];
    char line[USERNAME_MAX_LENGTH + PASSWORD_MAX_LENGTH + 2];

    #if ENCRYPT_PASSWORD 
        char salt[2];
    #else
    #endif
    

    char res[512]; // result of decryption operation

    // protecting shared resource with mutex semaphore
    pthread_mutex_lock(&users_lock_g);

    FILE* users_file;

    users_file = fopen(USER_FILE, "r");     // opening file in read mode
    if (users_file == NULL) {
        perror("fopen(USER_FILE)");

        pthread_mutex_unlock(&users_lock_g);
        return -1;       // file is missing...
    }


    // Scroll through the lines of users_file
    while(fgets(line, sizeof(line), users_file)){


        // get username and password from line
        sscanf(line, "%s %s", stored_username, stored_enc_psswd);

        #if ENCRYPT_PASSWORD 
            // retrieve salt
            salt[0] = stored_enc_psswd[0];
            salt[1] = stored_enc_psswd[1];

            
            // copy encrypted password to res and return it.
            #if 1
                strncpy(res, crypt(user->actual_password, salt), sizeof(res)); 
            #else
                memset(res, '\0', sizeof(res));
                strcpy(res, crypt(user->actual_password, salt)); 
            #endif
            
        #else
            strcpy(res, user->actual_password);
        
        #endif
        // if username and password match, login is successful.
        if (strcmp(user->username, stored_username) == 0 && strcmp(res, stored_enc_psswd) == 0){
            fclose(users_file);
            pthread_mutex_unlock(&users_lock_g);
            return 0;
        }
    }

    fclose(users_file);
    pthread_mutex_unlock(&users_lock_g);
    return 1;

}


int 
saveReservation(int thread_index, User* u, Booking* b)
{

    /* You may want to add a check to see whether the same 
     * reservation is already stored, even though it's higly unlikely
     * given that the code is random.
     */

    int rv;


    // manipulating date and putting into this form <yyyymmdd> which is easier to sort. remember d is <dd/mm>
    char date_yyyymmdd[9] = {'2','0','2','0',b->date[3],b->date[4],b->date[0],b->date[1]};


    char sql_command[256];

    memset(sql_command, '\0', sizeof(sql_command));

    // no SQL injection hazard, values are sanitized on client side.
    strcat(sql_command, "INSERT or IGNORE INTO Bookings(user, date, date_yyyymmdd, room, code) VALUES('");
                            // "or IGNORE" is actually negligible since I'm sure the room differs from any other room
                            // with the same user and date previously stored.
    strcat(sql_command, u->username);
    strcat(sql_command, "', '");
    strcat(sql_command, b->date);
    strcat(sql_command, "', '");
    strcat(sql_command, date_yyyymmdd);
    strcat(sql_command, "', '");
    strcat(sql_command, b->room);
    strcat(sql_command, "', '");
    strcat(sql_command, b->code);
    strcat(sql_command, "');");
    

    rv = commitToDatabase(thread_index, sql_command);

    return rv;  // 0 is OK, -1 is not.
}



char* 
assignRoom(int thread_index, char* date)
{

    char sql_command[512];
    memset(sql_command, '\0', sizeof(sql_command));

    strcat(sql_command, "SELECT count(id) from Bookings WHERE date = '");
    strcat(sql_command, date);
    strcat(sql_command, "'");


    // Querying the database with the command just created
    query_t* query = (query_t*) malloc(sizeof(query_t));
    // memset(&query, 0, sizeof query );


    // initialize that long variable
    query = queryDatabase(thread_index, 1, sql_command);



    // Checking the results of the query
    if (query->rv == 0){     // if return value is not 0, something went wrong.

        if (atoi(query->query_result) == 0){
            // hotel is actually empty on that date.
            // assigning room 1 which is obviously free.
            return "1";
        }
        else {

            /*
                
                -- most convoluted way I could come up with to find a free room.
                -- how this works:
                --       1) first part selects a room stacked on top of the previous ones
                --       2) second part looks for a room that has been reserved and then released
                --          leaving a gap between two adjactent reservations
                --       3) ifnull check is performed to make sure it doesn't return NULL 
                --          which would make MIN fail.
                --       4) the min of those two selects is choosen.
                --

                SELECT MIN
                (
                    (
                        SELECT ifnull
                        (

                            (
                                SELECT   (room + 1)
                                FROM Bookings
                                WHERE (
                                    (date = '<DATE_OF_INTEREST>') 
                                    AND 
                                    room+1 <= <hotel_max_available_rooms>
                                    AND 
                                    (room + 1 NOT IN 
                                        (SELECT DISTINCT room FROM Bookings WHERE date = '<DATE_OF_INTEREST>')
                                    )
                                )
                            ),
                            999
                        )
                    )
                    ,
                    (    
                        SELECT ifnull
                        (
                            (
                                SELECT   (room -1)
                                FROM Bookings
                                WHERE (
                                    (date = '<DATE_OF_INTEREST>') 
                                    AND 
                                    room-1 > 0
                                    AND 
                                    (room - 1 NOT IN 
                                        (SELECT DISTINCT room FROM Bookings WHERE date = '<DATE_OF_INTEREST>')
                                    )
                                )
                            ),
                            999  
                        )
                    )
                )

            */

            char hotel_max_available_rooms_string[4];
            sprintf(hotel_max_available_rooms_string, "%d", hotel_max_available_rooms);


            // pushing THAT query into `sql_command` string.

            memset(sql_command, '\0', sizeof(sql_command));

            strcat(sql_command, "SELECT MIN ( ( SELECT ifnull ( ( SELECT (room + 1) FROM Bookings WHERE ( (date = '");
            strcat(sql_command, date);
            strcat(sql_command, "') AND room+1 <= ");
            strcat(sql_command, hotel_max_available_rooms_string);
            strcat(sql_command, " AND (room + 1 NOT IN (SELECT DISTINCT room FROM Bookings WHERE date = '");
            strcat(sql_command, date);
            strcat(sql_command, "') ) ) ), 999 ) ) , ( SELECT ifnull ( ( SELECT (room -1) FROM Bookings WHERE ( (date = '");
            strcat(sql_command, date);
            strcat(sql_command, "') AND room-1 > 0 AND (room - 1 NOT IN (SELECT DISTINCT room FROM Bookings WHERE date = '");
            strcat(sql_command, date);
            strcat(sql_command, "') ) ) ), 999 ) ) )");

            query = queryDatabase(thread_index, 1, sql_command);

            // Checking the results of the query
            if (query->rv == 0){     // if return value is not 0, something went wrong.

                if (atoi(query->query_result) == 999){
                    // hotel is full
                    free(query);
                    return "FULL";
                }
                else {
                    // free(query); ... to be done
                    return query->query_result; // NUMBER as string
                }
            }
            else {
                printf("%s\n", "Error querying the database!");
                free(query);
                return "";
            }

        }
    }
    else {
        printf("%s\n", "Error querying the database!");
        free(query);
        return "";
    }
}   





void
generateRandomString(char* str, size_t size)  // size_t: type able to represent the size of any object in bytes
{

    const char charset[] =  "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                            "abcdefghijklmnopqrstuvwxyz"
                            "0123456789";                       // side note: the characters "." and "/" are also allowed as arguments of `crypt()`,
                                                                //            however they're not included in this list because the reservation code
                                                                //            -- that uses this function -- has to be an alphanumeric string.

    // init random number generator
    srand(time(NULL));
    

    if (size) {
        --size;
        for (size_t n = 0; n < size; n++) {
            int key = rand() % (int) (sizeof charset - 1);
            str[n] = charset[key];
        }
        // str[size] = '\0';
    }
    return;
}



char* 
fetchUserReservations(int thread_index, User* user)
{
    query_t* query = (query_t*) malloc(sizeof(query_t));
 
    char sql_command[1024];
    memset(sql_command, '\0', sizeof(sql_command));

    strcat(sql_command, "SELECT date, room, code FROM Bookings WHERE user = '");
    strcat(sql_command, user->username);
    #if SORT_VIEW_BY_DATE
        strcat(sql_command, "' ORDER BY date_yyyymmdd, room");
    #else
        strcat(sql_command, "' ORDER BY id");
    #endif
    

    query = queryDatabase(thread_index, 0, sql_command);


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
releaseReservation(int thread_index, User* user, Booking* booking)
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
    query = queryDatabase(thread_index, 2, sql_command);
    

    if (query->rv == 0){
        
        if ( *((int*) query->query_result) == 0){  //1 if in database, 0 otherwise.
            
            free(query);
            return -1;
        }

        else {

            // prepare payload for wiping that entry from the table

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

            // freeing memory before returning
            free(query);
            return commitToDatabase(thread_index, sql_command); // 0 is ok.

        }
    
    }
    else {
        printf("%s\n", "Error querying the database!");

        free(query);
        return -1;
    }

}

