/**
 * @name            hotel-booking
 * @file            client.c
 * @author          Francesco Urbani <https://urbanij.github.io/>
 *
 * @date            Mon Jul  1 12:43:37 CEST 2019
 * @brief           client side, main
 *
 * *compilation     `make client` or `gcc client.c -o client`
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
#include "messages.h"

#include "Address.h"
#include "Booking.h"
#include "Hotel.h"
#include "User.h"


// regex patterns
#define REGEX_HELP          "help"
#define REGEX_LOGIN         "login"
#define REGEX_REGISTER      "register"
#define REGEX_VIEW          "view"
#define REGEX_QUIT          "quit"
#define REGEX_LOGOUT        "logout"
#define REGEX_RESERVE       "reserve"
#define REGEX_RELEASE       "release"

#define REGEX_ROOM          "^[1-9]{1,3}$"          // room can be in range 1-999 
#define REGEX_CODE          "^([a-zA-Z0-9]{5})$"    // 5 chars, alphanumeric

// removed ^ from date regex pattern so leading whitespaces are discarded.
#define REGEX_DATE_FORMAT   "[0-9]{2}/[0-9]{2}$"
#define REGEX_DATE_VALID    "(((0[1-9]|[12][0-9]|3[01])/(0[13578]|1[02]))|((0[1-9]|[12][0-9]|30)/(0[13456789]|1[012]))|((0[1-9]|1[0-9]|2[0-8])/02)|(29/02))$"
                            /* inspired by [this answer](https://stackoverflow.com/a/21583100/6164816), I've then removed the year part and replaced "-" with "/".
                             */


/* . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . */


int 
main(int argc, char** argv) 
{

    // clear terminal
    system("clear");

    #if GDB_MODE
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

    

    // setup the client and return socket file descriptor.
    int sockfd = setupClient(&address);



    #ifdef MANAGE_CNTRL_C
        signal(SIGINT, closeConnection);
    #endif



    char command[BUFSIZE];
    char cmd[BUFSIZE];  // local command, used when parsing the reserve and release instructions
    char response[BUFSIZE];


    
    char username[USERNAME_MAX_LENGTH];
    char password[PASSWORD_MAX_LENGTH];




    User* user = (User*) malloc(sizeof(User));
    memset(user, '\0', sizeof(User));


    Booking* booking = (Booking*) malloc(sizeof(Booking));
    memset(booking, '\0', sizeof(Booking));


    
    // FSM initialization
    client_fsm_state_t state = CL_INIT;    


    // show available commands even before starting off.
    printf("%s\n", HELP_UNLOGGED_MESSAGE);    
    

    while (1) 
    {
        
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



                if      (regexMatch(command, REGEX_HELP)){
                    state = SEND_HELP;
                }
                else if (regexMatch(command, REGEX_LOGIN)){ 
                    state = SEND_LOGIN;
                }
                else if (regexMatch(command, REGEX_REGISTER)) {
                    state = SEND_REGISTER;
                }
                else if (regexMatch(command, REGEX_QUIT)){
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
                writeSocket(sockfd, HELP_MSG);  
                state = READ_HELP_RESP;
                break;

            case READ_HELP_RESP:
                // always cleaning data before reading/writing in/from it.
                memset(response, '\0', BUFSIZE);

                readSocket(sockfd, response);
                if (strcmp(response, "H") == 0){
                    printf("%s\n", HELP_UNLOGGED_MESSAGE);    
                }
                state = CL_INIT;
                break;

            case SEND_HELP_LOGGED:
                writeSocket(sockfd, HELP_MSG);
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
                writeSocket(sockfd, QUIT_MSG);
                printf("Quitting...\n");
                
                memset(command, '\0', sizeof(command));
                strcpy(command, "abort");
                break;

            case SEND_REGISTER:
                writeSocket(sockfd, REGISTER_MSG);
                state = READ_REGISTER_RESP;
                break;

            case READ_REGISTER_RESP:
                memset(command, '\0', sizeof(command));
                readSocket(sockfd, command);
                state = SEND_USERNAME;
                break;

            case SEND_USERNAME:
                printf("Choose username: ");
                fgets(username, USERNAME_MAX_LENGTH, stdin);          // `\n` is included in username thus I replace it with `\0`
                username[strlen(username)-1] = '\0';

                // convert to input to lower case (for easier later comparison)
                lower(username);

                if (strlen(username) < USERNAME_MIN_LENGTH || strlen(username) > USERNAME_MAX_LENGTH-1){
                    printf("Sorry, username length needs to be [%d-%d]\n", USERNAME_MIN_LENGTH, USERNAME_MAX_LENGTH);
                    state = SEND_USERNAME;
                    break;
                }
                
                writeSocket(sockfd, username); 

                // to be used later when i'm logged in and display the username
                strcpy(user->username, username); 

                state = READ_USERNAME_RESP;
                break;

            case READ_USERNAME_RESP:
                memset(command, '\0', sizeof(command));
                readSocket(sockfd, command);
                if (strcmp(command, "Y") == 0){
                    printf("Username OK.\n");
                }
                else {
                    printf(USERNAME_TAKEN_MSG);
                }

                if (strcmp(command, "Y") == 0){
                    state = SEND_PASSWORD;
                }
                else {
                    state = SEND_USERNAME;
                }
                break;

            case SEND_PASSWORD:

                readPassword(password);

                // check password length. If it exceedes the space this causes 
                // buffer overlow and crashes the server!
                if (strlen(password) < PASSWORD_MIN_LENGTH || strlen(password) > PASSWORD_MAX_LENGTH-1){
                    printf("Sorry, password length needs to be [%d-%d]\n", PASSWORD_MIN_LENGTH, PASSWORD_MAX_LENGTH);
                    state = SEND_PASSWORD;
                    break;
                }

                writeSocket(sockfd, password);

                state = READ_PASSWORD_RESP;
                break;
            
            case READ_PASSWORD_RESP:
                memset(command, '\0', sizeof(command));
                readSocket(sockfd, command);  // OK: Account was successfully setup.
                printf("%s\n", command);

                memset(command, '\0', sizeof(command));
                readSocket(sockfd, command);  // Successfully registerd, you are now logged in
                printf("%s\n", command);

                state = CL_LOGIN;
                break;

            case SEND_LOGIN:
                writeSocket(sockfd, LOGIN_MSG);         
                state = READ_LOGIN_RESP;
                break;

            case READ_LOGIN_RESP:
                memset(command, '\0', sizeof(command));
                readSocket(sockfd, command);    // "OK"
                state = SEND_LOGIN_USERNAME;
                break;

            case SEND_LOGIN_USERNAME:
                printf(USERNAME_PROMPT_MSG);
                fgets(username, USERNAME_MAX_LENGTH, stdin);          // `\n` is included in username thus I replace it with `\0`
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
                    printf("OK.\n");
                    state = SEND_LOGIN_PASSWORD;
                }
                else {
                    printf(UNREGISTERED_USERNAME_ERR_MSG);
                    state = CL_INIT;
                }
                break;

            case SEND_LOGIN_PASSWORD:

                readPassword(password);

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
                    printf(ACCESS_GRANTED_MSG);
                    state = CL_LOGIN;
                }
                else {
                    printf(WRONG_PASSWORD_MSG);
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


                if      (regexMatch(command, REGEX_HELP)){
                    state = SEND_HELP_LOGGED;
                }
                else if (regexMatch(command, REGEX_VIEW)) {
                    state = SEND_VIEW;  
                }
                else if (regexMatch(command, REGEX_QUIT)){
                    state = SEND_QUIT;
                }
                else if (regexMatch(command, REGEX_LOGOUT)){
                    state = SEND_LOGOUT;
                }
                else {
                    // splitting input in its parts.

                    memset(cmd,           '\0', sizeof cmd);
                    memset(booking->date, '\0', sizeof booking->date);
                    memset(booking->room, '\0', sizeof booking->room);
                    memset(booking->code, '\0', sizeof booking->code);

                    sscanf(command, "%s %s %s %s", 
                        cmd, booking->date, booking->room, booking->code
                    ); 


                    if (regexMatch(cmd, REGEX_RESERVE)){
                        if (regexMatch(booking->date, REGEX_DATE_FORMAT)){
                            if (regexMatch(booking->date, REGEX_DATE_VALID)){
                                state = SEND_RESERVE;        
                            } else {
                                printf(INVALID_DATE_MSG);
                                state = CL_LOGIN;
                            }
                            
                        }
                        else{
                            state = INVALID_DATE;
                        }
                    }
                    else if (regexMatch(cmd, REGEX_RELEASE)){

                        if (regexMatch(booking->date, REGEX_DATE_FORMAT) && 
                            regexMatch(booking->room, REGEX_ROOM) &&
                            regexMatch(booking->code, REGEX_CODE))
                        {
                            if (regexMatch(booking->date, REGEX_DATE_VALID)){
                                state = SEND_RELEASE;
                            }
                            else {
                                printf(INVALID_DATE_MSG);
                                state = CL_LOGIN;
                            }
                            
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
                printf(INVALID_FORMAT_RESERVE_MSG);
                state = CL_LOGIN;
                break;

            case INVALID_RELEASE:
                printf(INVALID_FORMAT_RELEASE_MSG);
                state = CL_LOGIN;
                break;


            case SEND_LOGOUT:
                writeSocket(sockfd, LOGOUT_MSG);
                state = CL_INIT;

                // to be used later when i'm logged in and display the username
                memset(user, '\0', sizeof(User));

                break;

            case SEND_RESERVE:
                writeSocket(sockfd, RESERVE_MSG);
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

                    printf("\033[92mReservation successful:\x1b[0m room %s, code %s\n", 
                        booking->room, booking->code
                    );
                
                }
                state = CL_LOGIN;
                break;


            case SEND_VIEW:
                writeSocket(sockfd, VIEW_MSG);
                state = READ_VIEW_RESP;
                break;

            case READ_VIEW_RESP:
                memset(response, '\0', BUFSIZE);
                readSocket(sockfd, response);
                printf("%s\n", response);
                state = CL_LOGIN;
                break;

            case SEND_RELEASE:
                writeSocket(sockfd, RELEASE_MSG);
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
            
        } // end switch
        
        printClientFSMState(&state);

        // checking the only condition that would let me quit the infinite loop.
        if (strcmp(command, "abort") == 0){ 
            break;
        }

    } // end while

    
    free(user);
    free(booking);

    close(sockfd);

    return 0;
}


