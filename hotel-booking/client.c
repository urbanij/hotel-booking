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


/* POSIX libraries */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>         // getpass()

// networking
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// miscellaneous
#include <regex.h>
#include <signal.h>         // signal()



/* user-defined headers */

// config definition and declarations
#include "config.h"

#include "utils.h"

#include "Address.h"
#include "Hotel.h"
#include "User.h"


// regex patterns
#define REGEX_DATE "^(((0[1-9]|[12][0-9]|3[01])/(0[13578]|1[02]))|((0[1-9]|[12][0-9]|30)/(0[13456789]|1[012]))|((0[1-9]|1[0-9]|2[0-8])-02)|(29-02))$"
#define REGEX_ROOM "^[1-9]{1,2}$"
#define REGEX_CODE "^([a-zA-Z0-9]{5})$"



#if HELP_MESSAGE_TYPE_1

    // Only show the actual options

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
#else

    // show ALL the options no matter what

    #define HELP_UNLOGGED_MESSAGE "Commands:\n\
            \x1b[36m help                         \x1b[0m --> show commands\n\
            \x1b[36m register                     \x1b[0m --> register an account\n\
            \x1b[36m login                        \x1b[0m --> log into the system\n\
            \x1b[36m quit                         \x1b[0m --> quit\n\
            \x1b[36m logout                       \x1b[0m --> log out                 (log-in required)\n\
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
#endif
#define INVALID_COMMAND_MESSAGE "\x1b[31mInvalid command.\x1b[0m\n"



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
int checkPasswordValidity();

/** @brief
 *  @param
 *  @param
 *  @return
 */
int match(const char* string, const char* pattern);

/* . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . */


int 
main(int argc, char** argv) {

    // clear terminal
    system("clear");

    #ifdef GDB_MODE
        char port[5];
        printf("Insert port number: ");
        scanf("%s", port);

        Address address = (Address){
            .ip   = "127.0.0.1",
            .port = atoi(port)
        };
        
    #else
        // reading arguments from stdin
        Address address = readArguments(argc, argv);
    #endif 

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


    #if 0
        char username[20];
        char password[PASSWORD_MAX_LENGTH];
    #else
        // getpass() function seems to work only with dynamic strings
        char* username = (char*) malloc(20 * sizeof(char));
        char* password = (char*) malloc(PASSWORD_MAX_LENGTH * sizeof(char));
    #endif




    User* user = (User*) malloc(sizeof(User));
    memset(user, '\0', sizeof(User));


    Booking* booking = (Booking*) malloc(sizeof(Booking));
    memset(booking, '\0', sizeof(Booking));


    #define LOCK_INFINITE_CYCLE 0
    #if LOCK_INFINITE_CYCLE
        int temporary_cycle_counter = 0;
    #endif


    // char input_string[40];


    


    state = CL_INIT;

    int rv;
    

    while (1) 
    {
        #if LOCK_INFINITE_CYCLE
            temporary_cycle_counter ++;

            if (temporary_cycle_counter > 20) break;    // prevent inf loop
        #endif 


        switch (state)
        {
            case CL_INIT:
                printf("> ");
                
                // read input
                memset(command, '\0', BUFSIZE);
                fgets(command, 20, stdin);

                // drop the new line and replace w/ the string termination
                command[strlen(command)-1] = '\0';
                
                // convert to input to lower case (for easier later comparison)
                lower(command);



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
                printf(INVALID_COMMAND_MESSAGE);
            
                state = CL_INIT;
                break;
            

            case SEND_HELP:
                writeSocket(sockfd, "h");   // trims the command to the first char only
                                            // since it uniquely indentifies the command. 
                                            // there's not need to ship the whole string
            
                state = READ_HELP_RESP;
                break;

            case READ_HELP_RESP:
                memset(response, '\0', BUFSIZE);
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
                memset(response, '\0', BUFSIZE);
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
                memset(command, '\0', sizeof(command));
                readSocket(sockfd, command);    // Choose username
                // printf("%s", command);
                state = SEND_USERNAME;
                break;

            case SEND_USERNAME:
                printf("Choose username: ");
                fgets(username, 20, stdin);          // `\n` is included in username thus I replace it with `\0`
                username[strlen(username)-1] = '\0';

                // convert to input to lower case (for easier later comparison)
                lower(username);
                
                writeSocket(sockfd, username); 

                // to be used later when i'm logged in and display the username
                strcpy(user->username, username); 

                state = READ_USERNAME_RESP;
                break;

            case READ_USERNAME_RESP:
                memset(command, '\0', sizeof(command));
                readSocket(sockfd, command);
                if (strcmp(command, "Y") == 0){
                    printf("%s\n", "username OK.");
                }
                else {
                    printf("%s\n", "\x1b[31m\033[1musername already taken.\x1b[0m");
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

                // memset(password, '\0', sizeof(password));
                password = getpass("Choose password: ");

                // check password length. If it exceedes the space this causes 
                // buffer overlow and crashes the server!
                if (strlen(password) < 4 || strlen(password) > PASSWORD_MAX_LENGTH-1){
                    printf("Sorry, password length needs to be [4-%d]\n", PASSWORD_MAX_LENGTH);
                    state = SEND_PASSWORD;
                    break;
                }


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
                memset(command, '\0', sizeof(command));
                readSocket(sockfd, command);  // OK: Account was successfully setup.
                printf("%s\n", command);
                // memset(command, '\0', BUFSIZE);

                memset(command, '\0', sizeof(command));
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
                memset(command, '\0', sizeof(command));
                readSocket(sockfd, command);    // "OK"
                // printf("%s\n", command);
                state = SEND_LOGIN_USERNAME;
                break;

            case SEND_LOGIN_USERNAME:
                printf("Insert username: ");
                fgets(username, 20, stdin);          // `\n` is included in username thus I replace it with `\0`
                username[strlen(username)-1] = '\0';

                lower(username);
                writeSocket(sockfd, username);

                // to be used later when i'm logged in and display the username
                strcpy(user->username, username); 
                
                state = READ_LOGIN_USERNAME_RESP;
                break;

            case READ_LOGIN_USERNAME_RESP:
                memset(command, '\0', sizeof(command));
                readSocket(sockfd, command);
                if (strcmp(command, "Y") == 0){
                    printf("%s\n", "OK.");
                    state = SEND_LOGIN_PASSWORD;
                }
                else {
                    printf("%s\n", "\x1b[31mUnregistered username.\x1b[0m\nGo ahead and register first.");
                    //                ^--red  ^--bold
                    state = CL_INIT;
                }
                break;

            case SEND_LOGIN_PASSWORD:
                password = getpass("Insert password: ");

                /* Check password length. If password causes buffer overflow
                 * the server trims it and accept it. You don't want that. 
                 * Prevent this from happening on the client side by forcing the user
                 * to retype the password.
                 */
                if (strlen(password) < 4 || strlen(password) > PASSWORD_MAX_LENGTH-1){
                    printf("\x1b[31m\033[1mwrong password.\x1b[0m Try again.\n");
                    state = SEND_LOGIN_PASSWORD;
                    break;
                }

                writeSocket(sockfd, password);
                state = READ_LOGIN_PASSWORD_RESP;
                break;

            case READ_LOGIN_PASSWORD_RESP:
                memset(command, '\0', sizeof(command));
                readSocket(sockfd, command);
                if (strcmp(command, "Y") == 0){
                    printf("%s\n", "OK, access granted.");
                    state = CL_LOGIN;
                }
                else {
                    printf("%s\n", "\x1b[31m\033[1mwrong password.\x1b[0m Try to login again...");
                    //                ^--red  ^--bold
                    state = CL_INIT;
                }
                break;

            case CL_LOGIN:
                
                printf(ANSI_COLOR_YELLOW ANSI_BOLD "(%s)" ANSI_COLOR_RESET "> ", user->username);

                // read input
                memset(command, '\0', BUFSIZE);
                fgets(command, 30, stdin);

            
                // drop the new line and replace w/ the string termination
                command[strlen(command)-1] = '\0';
                
                // convert to input to lower case (for easier later comparison)
                lower(command);

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

                    memset(cmd,           '\0', sizeof(cmd)    );
                    memset(booking->date, '\0', sizeof booking->date);
                    memset(booking->room, '\0', sizeof booking->room);
                    memset(booking->code, '\0', sizeof booking->code);

                    sscanf(command, "%s %s %s %s", 
                        cmd, booking->date, booking->room, booking->code
                    ); 


                    if (strcmp(cmd, "reserve") == 0){
                        if (match(booking->date, REGEX_DATE))
                            state = SEND_RESERVE;    
                        else
                            state = INVALID_DATE;
                    }
                    else if (strcmp(cmd, "release") == 0){

                        if (match(booking->date, REGEX_DATE) && 
                            match(booking->room, REGEX_ROOM) &&
                            match(booking->code, REGEX_CODE)){

                            state = SEND_RELEASE;    
                        }
                        else {
                            state = INVALID_RELEASE;
                        }
                    
                    }
                    else {
                        state = INVALID_LOGGED_IN;
                    }
                }
                break;

            case INVALID_LOGGED_IN:
                printf(INVALID_COMMAND_MESSAGE);
                state = CL_LOGIN;
                
                break;

            case INVALID_DATE:
                printf("\x1b[31mInvalid format.\x1b[0m Make sure the foramt is:\n\t        reserve [gg/mm]\n");
                state = CL_LOGIN;
                break;

            case INVALID_RELEASE:
                printf("\x1b[31mInvalid format.\x1b[0m Make sure the format is:\n\t        release [gg/mm] [room] [code]\n");
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

                state = READ_RESERVE_RESP;
                break;
            
            case READ_RESERVE_RESP:
                memset(command, '\0', sizeof(command));
                readSocket(sockfd, command);

                if (strcmp(command, "BADDATE") == 0){
                    printf("%s\n", "Wrong date format");
                }
                else if (strcmp(command, "NOAVAL") == 0){
                    printf("\x1b[31mNo room available on %s/2020\x1b[0m\n", booking->date);
                }
                else if (strcmp(command, "RESOK") == 0){
                    memset(booking->room, '\0', sizeof(booking->room));
                    memset(booking->code, '\0', sizeof(booking->code));

                    readSocket(sockfd, booking->room);
                    readSocket(sockfd, booking->code);

                    printf("\033[92mReservation successful:\x1b[0m room %s, code %s.\n", 
                        booking->room, booking->code
                    );
                
                }
                state = CL_LOGIN;
                break;


            case SEND_VIEW:
                writeSocket(sockfd, "v");
                state = READ_VIEW_RESP;
                break;

            case READ_VIEW_RESP:
                memset(response, '\0', BUFSIZE);
                readSocket(sockfd, response);
                // printf("%s\n", "Your active reservations:");
                printf("%s\n", response);
                state = CL_LOGIN;
                break;

            case SEND_RELEASE:
                writeSocket(sockfd, "rel");
                writeSocket(sockfd, booking->date);
                writeSocket(sockfd, booking->room);
                writeSocket(sockfd, booking->code);

                state = READ_RELEASE_RESP;
                break;

            case READ_RELEASE_RESP:
                memset(response, '\0', BUFSIZE);
                readSocket(sockfd, response);

                printf("%s\n", response);
                state = CL_LOGIN;

                break;


            // there is no default, every state transition is defined.
            #if 0
            default:
                state = CL_INIT;
            #endif

            
        }
        
        printClientFSMState(&state);

    }


ABORT:

    #if 1
        free(username);
        free(password);
    #endif


    free(user);
    free(booking);


    close(sockfd);

    return 0;
}



/* . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . */


int match(const char* string, const char* pattern)
{
    regex_t re;
    if (regcomp(&re, pattern, REG_EXTENDED|REG_NOSUB) != 0) return 0;
    int status = regexec(&re, string, 0, NULL, 0);
    regfree(&re);
    if (status != 0) return 0;
    return 1;
}


int 
checkPasswordValidity(){
    /*  */
    return 0;
}



