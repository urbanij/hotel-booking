/**
 * @project:        hotel-booking
 * @file:           client.c
 * @author(s):      Francesco Urbani <https://urbanij.github.io/>
 *
 * @date:           Mon Jul  1 12:43:37 CEST 2019
 * @Description:    client side, main
 *
 * @compilation:    `make client` or `gcc client.c -o client`
 */

/*      available commmands  
 *      ---------------------+
 *      help                
 *      register            
 *      login               
 *      view                
 *      quit                
 *      logout
 *      reserve    [date]
 *      release    [date] [room] [code]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// networking
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// miscellaneous
#include <signal.h>         // signal
#include <ctype.h>

// config definition and declarations
#include "config.h"

#include "utils.h"

#include "Address.h"
#include "Hotel.h"
#include "User.h"



#if HELP_MESSAGE_TYPE_1
    #define HELP_UNLOGGED_MESSAGE "Commands:\n\
            \x1b[36m help                         \x1b[0m --> show commands\n\
            \x1b[36m register                     \x1b[0m --> register an account\n\
            \x1b[36m login                        \x1b[0m --> log into the system\n\
            \x1b[36m quit                         \x1b[0m --> quit\n\
            \x1b[36m logout                       \x1b[0m --> log out\n               (log-in required)\n\
            \x1b[36m reserve [date]               \x1b[0m --> book a room             (log-in required)\n\
            \x1b[36m release [date] [room] [code] \x1b[0m --> cancel a booking        (log-in required)\n\
            \x1b[36m view                         \x1b[0m --> show current bookings   (log-in required)\n"
    #define HELP_LOGGED_IN_MESSAGE "Commands:\n\
            \x1b[36m help                         \x1b[0m --> show commands\n\
            \x1b[36m reserve [date]               \x1b[0m --> book a room\n\
            \x1b[36m release [date] [room] [code] \x1b[0m --> cancel a booking\n\
            \x1b[36m view                         \x1b[0m --> show current bookings\n\
            \x1b[36m logout                       \x1b[0m --> log out\n\
            \x1b[36m quit                         \x1b[0m --> log out and quit\n\
            \x1b[36m register                     \x1b[0m --> register an account     (you have to be logged-out)\n\
            \x1b[36m login                        \x1b[0m --> log into the system     (you have to be logged-out)\n"
#else

    #define HELP_UNLOGGED_MESSAGE "Commands:\n\
            \x1b[36m help     \x1b[0m --> show commands\n\
            \x1b[36m register \x1b[0m --> register an account\n\
            \x1b[36m login    \x1b[0m --> log into the system\n\
            \x1b[36m quit     \x1b[0m --> log out and quit\n"
    #define HELP_LOGGED_IN_MESSAGE "Commands:\n\
            \x1b[36m help                         \x1b[0m --> show commands\n\
            \x1b[36m reserve [date]               \x1b[0m --> book a room\n\
            \x1b[36m release [date] [room] [code] \x1b[0m --> cancel a booking\n\
            \x1b[36m view                         \x1b[0m --> show current bookings\n\
            \x1b[36m logout                       \x1b[0m --> log out\n\
            \x1b[36m quit                         \x1b[0m --> log out and quit\n"
#endif



/********************************/
/*                              */
/*          global var          */
/*                              */
/********************************/

static client_fsm_state_t  state;

/********************************/
/*                              */
/*          protoypes           */
/*                              */
/********************************/





/********************************/
/*                              */
/*          functions           */
/*                              */
/********************************/

/** @brief
 *  @param
 *  @param
 *  @return
 */
int checkIfUserAlreadyLoggedIn();


/** @brief
 *  @param
 *  @param
 *  @return
 */
int checkPasswordValidity();


/* . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . */


int 
main(int argc, char** argv) {

    // reading arguments from stdin
    Address address = readArguments(argc, argv);
    repr_addr(&address);   // print address
    printf("connected.");

    // setup the client and return socket file descriptor.
    int sockfd = setupClient(&address);

    #ifdef MANAGE_CNTRL_C
        signal(SIGINT, closeConnection);
    #endif



    char command[BUFSIZE];
    char cmd[BUFSIZE];  // local command, used when parsing the reserve and release instructions
    char response[BUFSIZE];

    // char username[20];
    // char password[30];

    User* user = (User*) malloc(sizeof(User));
    memset(user, '\0', sizeof(User));


    Booking* booking = (Booking*) malloc(sizeof(Booking));
    memset(booking, '\0', sizeof(Booking));


    #define LOCK_INFINITE_CYCLE 0
    #if LOCK_INFINITE_CYCLE
        int temporary_cycle_counter = 0;
    #endif


    // char input_string[40];


    char* username = (char*) malloc(20 * sizeof(char));
    char* password = (char*) malloc(20 * sizeof(char));

    state = CL_INIT;

    int rv;
    

    while (1) 
    {
        #if LOCK_INFINITE_CYCLE
            temporary_cycle_counter ++;

            if (temporary_cycle_counter > 20) break;    // prevent inf loop
        #endif 



        memset(command, '\0', BUFSIZE);
        memset(response, '\0', BUFSIZE);


        switch (state)
        {
            case CL_INIT:
                printf("> ");
                
                // read input
                fgets(command, 20, stdin);

                // drop the new line and replace w/ the string termination
                command[strlen(command)-1] = '\0';
                
                // convert to input to lower case (for easier later comparison)
                for (char *p = command; *p; ++p){
                    *p = *p >= 0x41 && *p <= 0x5A ? (*p | 0x60) : *p;
                    //          (A)           (Z)
                }


                if      (strcmp(command, "help") == 0){
                    state = SEND_HELP;
                }
                else if (strcmp(command, "login") == 0){ 
                    state = SEND_LOGIN;
                }
                else if (strcmp(command, "register") == 0) {
                    state = SEND_REGISTER;
                }
                else if (strcmp(command, "quit") == 0){
                    state = SEND_QUIT;
                }
                else{
                    state = INVALID_UNLOGGED;
                }
                
                break;



            case INVALID_UNLOGGED:
                /* command doesn't match anything the server is supposed 
                 * to receive hence i don't send anything into the socket.
                 */
                printf("\x1b[33mInvalid command.\x1b[0m \n");
            
                state = CL_INIT;
                break;
            

            case SEND_HELP:
                writeSocket(sockfd, "h");   // trims the command to the first char only
                                            // since it uniquely indentifies the command. 
                                            // there's not need to ship the whole string
            
                state = READ_HELP_RESP;
                break;

            case READ_HELP_RESP:
                readSocket(sockfd, response);
                if (strcmp(response, "H") == 0){
                    printf("%s\n", HELP_UNLOGGED_MESSAGE);    
                }
                state = CL_INIT;
                break;

            case SEND_HELP_LOGGED:
                writeSocket(sockfd, "hh");
            
                state = READ_HELP_LOGGED_RESP;
                break;

            case READ_HELP_LOGGED_RESP:
                readSocket(sockfd, response);
                if (strcmp(response, "H") == 0){
                    printf("%s\n", HELP_LOGGED_IN_MESSAGE);    
                }
                state = CL_LOGIN;
                break;



            case SEND_QUIT:
                writeSocket(sockfd, "q");
                printf( "You quit the system.\n");
                goto ABORT;

                state = CL_INIT;   // you sure ?

            case SEND_REGISTER:
                writeSocket(sockfd, "r");
            
                state = READ_REGISTER_RESP;
                break;

            case READ_REGISTER_RESP:
                readSocket(sockfd, command);    // Choose username
                printf("%s", command);
            
                state = SEND_USERNAME;
                break;

            case SEND_USERNAME:
                printf("> ");
                fgets(username, 20, stdin);          // `\n` is included in username thus I replace it with `\0`
                username[strlen(username)-1] = '\0';
                
                writeSocket(sockfd, username); 

                // to be used later when i'm logged in and display the username
                strcpy(user->username, username); 

                state = READ_USERNAME_RESP;
                break;

            case READ_USERNAME_RESP:
                readSocket(sockfd, command);
                if (strcmp(command, "Y") == 0){
                    printf("%s\n", "username OK.");
                }
                else {
                    printf("%s\n", "\x1b[31m\033[1musername already taken.\x1b[0m\npick another one: ");
                    //                ^--red  ^--bold
                }

                if (strcmp(command, "Y") == 0){
                    state = SEND_PASSWORD;
                }
                else {
                    state = SEND_USERNAME;
                }
                break;

            case SEND_PASSWORD:
                password = getpass("Choose password: ");

                writeSocket(sockfd, password);
            
                rv = checkPasswordValidity(); // 0 is OK, -1 invalid
                if (rv == 0){
                    state = READ_PASSWORD_RESP;
                }
                else {
                    state = SEND_PASSWORD;
                }
                break;
            
            case READ_PASSWORD_RESP:
                readSocket(sockfd, command);  // OK: Account was successfully setup.
                printf("%s\n", command);
                // memset(command, '\0', BUFSIZE);

                readSocket(sockfd, command);  // Successfully registerd, you are now logged in
                printf("%s\n", command);
                // memset(command, '\0', BUFSIZE);

                state = CL_LOGIN;
                break;

            case SEND_LOGIN:
                writeSocket(sockfd, "l");         
                state = READ_LOGIN_RESP;
                break;

            case READ_LOGIN_RESP:
                readSocket(sockfd, command);    // "OK"
                // printf("%s\n", command);
                state = SEND_LOGIN_USERNAME;
                break;

            case SEND_LOGIN_USERNAME:
                printf("Insert username: ");
                fgets(username, 20, stdin);          // `\n` is included in username thus I replace it with `\0`
                username[strlen(username)-1] = '\0';
                writeSocket(sockfd, username);

                // to be used later when i'm logged in and display the username
                strcpy(user->username, username); 
                
                state = READ_LOGIN_USERNAME_RESP;
                break;

            case READ_LOGIN_USERNAME_RESP:
                readSocket(sockfd, command);
                if (strcmp(command, "Y") == 0){
                    printf("%s\n", "OK.");
                    state = SEND_LOGIN_PASSWORD;
                }
                else {
                    printf("%s\n", "\x1b[31m\033[1munregistered username.\x1b[0m\nGo ahead and register first.");
                    //                ^--red  ^--bold
                    state = CL_INIT;
                }
                break;

            case SEND_LOGIN_PASSWORD:
                password = getpass("Insert password: ");
                writeSocket(sockfd, password);
                state = READ_LOGIN_PASSWORD_RESP;
                break;

            case READ_LOGIN_PASSWORD_RESP:
                readSocket(sockfd, command);
                if (strcmp(command, "Y") == 0){
                    printf("%s\n", "OK, access granted.");
                    state = CL_LOGIN;
                }
                else {
                    printf("%s\n", "\x1b[31m\033[1mwrong password.\x1b[0m Try again from start.");
                    //                ^--red  ^--bold
                    state = CL_INIT;
                }
                break;

            case CL_LOGIN:
                printf("\033[92m(%s)\x1b[0m> ", user->username);
                
                // read input
                fgets(command, 20, stdin);

                // drop the new line and replace w/ the string termination
                command[strlen(command)-1] = '\0';
                
                // convert to input to lower case (for easier later comparison)
                for (char *p = command; *p; ++p){
                    *p = *p >= 0x41 && *p <= 0x5A ? (*p | 0x60) : *p;
                }
                
                #if 0
                    printBooking(booking);
                #endif
                /////////////////////////////////////////////////

            
                if      (strcmp(command, "help") == 0){
                    state = SEND_HELP_LOGGED;
                }
                else if (strcmp(command, "view") == 0) {
                    state = SEND_VIEW;  
                }
                else if (strcmp(command, "quit") == 0){
                    state = SEND_QUIT;
                }
                else if (strcmp(command, "logout") == 0){
                    state = SEND_LOGOUT;
                }
                else {
                    // splitting input in its parts.
                    sscanf(command, "%s %s %s %s", cmd, booking->date, booking->room, booking->code); 

                    if (strcmp(cmd, "reserve") == 0){
                        if (strlen(booking->date) >= 3)
                            state = SEND_RESERVE;    
                        else
                            state = INVALID_LOGGED_IN;
                    }
                    else {
                        state = INVALID_LOGGED_IN;
                    }
                }
                break;

            case INVALID_LOGGED_IN:
                printf("\x1b[33mInvalid command.\x1b[0m \n");
                state = CL_LOGIN;
                
                break;


            case SEND_LOGOUT:
                writeSocket(sockfd, "logout");
                state = CL_INIT;

                // to be used later when i'm logged in and display the username
                memset(user, '\0', sizeof(User));

                break;

            case SEND_RESERVE:
                writeSocket(sockfd, "res");

                writeSocket(sockfd, booking->date);

                memset(booking->date, '\0', sizeof(booking->date));


                state = READ_RESERVE_RESP;
                break;
            
            case READ_RESERVE_RESP:
                readSocket(sockfd, command);
                if (strcmp(command, "BADDATE") == 0){
                    printf("%s\n", "Wrong date format");
                }
                else if (strcmp(command, "NOAVAL") == 0){
                    printf("%s\n", "Sorry - Sold out");
                }
                else if (strcmp(command, "RESOK") == 0){
                    printf("%s\n", "\033[92mReservation successful.\x1b[0m");
                }
                state = CL_LOGIN;
                break;


            case SEND_VIEW:
                writeSocket(sockfd, "v");
                state = READ_VIEW_RESP;
                break;

            case READ_VIEW_RESP:
                readSocket(sockfd, response);
                printf("%s\n", response);
                state = CL_LOGIN;
                break;

            default:
                state = CL_INIT;

            
        }
        
        printClientFSMState(&state);

    }

    free(username);
    free(password);

    free(user);         // not sure it s right position
    free(booking);


    ABORT:
    close(sockfd);

    return 0;
}



/* . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . */





int 
checkIfUserAlreadyLoggedIn(){
    /*  */
    return 0;
}


int 
checkPasswordValidity(){
    /*  */
    return 0;
}



