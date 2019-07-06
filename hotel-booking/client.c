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


/*      available commmands  |  shortcuts
 *      ---------------------+---------------
 *      help                 |  h
 *      register             |  r
 *      login                |  l
 *      view                 |  v
 *      quit                 |  q
 *
 *      reserve              |  res   [date]
 *      release              |  rel   [date] [room] [code]
 */

#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
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
            \x1b[36m help     \x1b[0m --> show commands\n\
            \x1b[36m register \x1b[0m --> register an account\n\
            \x1b[36m login    \x1b[0m --> log into the system\n\
            \x1b[36m quit     \x1b[0m --> quit\n\
            \x1b[36m logout   \x1b[0m --> log out\n               (log-in required)\n\
            \x1b[36m reserve  \x1b[0m --> book a room             (log-in required)\n\
            \x1b[36m release  \x1b[0m --> cancel a booking        (log-in required)\n\
            \x1b[36m view     \x1b[0m --> show current bookings   (log-in required)\n"
    #define HELP_LOGGED_IN_MESSAGE "Commands:\n\
            \x1b[36m help     \x1b[0m --> show commands\n\
            \x1b[36m reserve  \x1b[0m --> book a room\n\
            \x1b[36m release  \x1b[0m --> cancel a booking\n\
            \x1b[36m view     \x1b[0m --> show current bookings\n\
            \x1b[36m logout   \x1b[0m --> log out\n\
            \x1b[36m quit     \x1b[0m --> log out and quit.\n\
            \x1b[36m register \x1b[0m --> register an account     (you have to be logged-out)\n\
            \x1b[36m login    \x1b[0m --> log into the system     (you have to be logged-out)\n"
#else

    #define HELP_UNLOGGED_MESSAGE "Commands:\n\
            \x1b[36m help     \x1b[0m --> show commands\n\
            \x1b[36m register \x1b[0m --> register an account\n\
            \x1b[36m login    \x1b[0m --> log into the system\n\
            \x1b[36m quit     \x1b[0m --> log out and quit.\n"

    #define HELP_LOGGED_IN_MESSAGE "Commands:\n\
            \x1b[36m help     \x1b[0m --> show commands\n\
            \x1b[36m reserve  \x1b[0m --> book a room\n\
            \x1b[36m release  \x1b[0m --> cancel a booking\n\
            \x1b[36m view     \x1b[0m --> show current bookings\n\
            \x1b[36m logout   \x1b[0m --> log out\n\
            \x1b[36m quit     \x1b[0m --> log out and quit.\n"
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

/**
 *
 */
int checkIfUserAlreadyLoggedIn();


/**
 *
 */
int checkPasswordValidity();


/* . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . */


int main(int argc, char** argv)
{

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
    char response[BUFSIZE];

    // char username[20];
    // char password[30];

    User* user = (User*) malloc(sizeof(User));
    memset(user, '\0', sizeof(User));


    #define LOCK_INFINITE_CYCLE 0
    #if LOCK_INFINITE_CYCLE
        int temporary_cycle_counter = 0;
    #endif



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


        #if 0
        memset(command, '\0', BUFSIZE);
        memset(response, '\0', BUFSIZE);

        printf("> ");

        #if 1
            
            #define USE_SCANF_AND_DISCARD_NEW_LINE    1
            #if USE_SCANF_AND_DISCARD_NEW_LINE
                scanf("%[^\n]%*c", command);    // https://stackoverflow.com/a/6282236/6164816
                                                // while the input is not a newline ('\n') take input.
                                                // Then with the %*c it reads the newline character
                                                // from the input buffer (which is not read), and the *
                                                // indicates that this read in input is discarded
            #else
                char string[BUFSIZE];
                fgets(string, BUFSIZE, stdin); // fgets function leaves '\n' in string, you gotta deal with it

                /* dealing with newline and replacing it with null terminator */
                // if ((strlen(string) > 0) && (string[strlen(string) - 1] == '\n'))
                // {
                //     string[strlen(string) - 1] = '\0';
                // }

                sscanf(string, "%s", command);     // sscanf: reads formatted input as string

            #endif
        #else
            char* reservation_date, total_rooms, reservation_code;
            int ret;
            ret = sscanf("%s %s %s %s", command, reservation_date, total_rooms, reservation_code);
        #endif


        // allocate space for storing array of strings which represent the args of the command.
        char** command_arguments = malloc(4 * sizeof(10 * sizeof(char)));

        // sensitive to white spaces
        char* pch = strtok(command," ");    /* C library function char *strtok(char *str, const char *delim) 
                                             * breaks string str into a series of tokens using the delimiter delim.
                                             * str     The contents of this string are modified and broken into 
                                             *         smaller strings (tokens).
                                             * delim   This is the C string containing the delimiters. These may vary from one call to another.
                                             */

        int arg_index = 0;
        while (pch != NULL){
            command_arguments[arg_index] = pch;
            pch = strtok (NULL, " ");
            arg_index++;
        }

        int arg_count = arg_index;
        #ifdef PRINT_ARGUMENTS_JUST_INSERTED
            for (int i = 0; i < arg_count; i++) {
                printf("%s\n", command_arguments[i]);
            }
        #endif


        if (
            (!strcmp (command, "help"    )) ||
            (!strcmp (command, "register")) ||
            (!strcmp (command, "login"   )) ||
            (!strcmp (command, "view"    ))
            ) {
            command[1] = '\0';              // trims the command to the first char only
            writeSocket(sockfd, command);   // since it uniquely indentifies the command. 
                                            // there's not need to ship the whole string

            readSocket(sockfd, response);
        }

        else if (!strcmp (command, "quit")) {
            int ret = closeConnection(sockfd);
            if (ret == 0) 
                return 0;
            else
                return -1;
        }

        else if( !strncmp(command, "reserve", strlen("reserve")) && arg_count == 2) {
            command[3] = '\0';
            writeSocket(sockfd, command);               // i.e. "res"
            writeSocket(sockfd, command_arguments[1]);  // reservation date
        }

        
        /* if command starts with release, also checks whether the arguments it's 
         * supposed to have are correct 
         */
        else if( !strncmp(command, "release", strlen("release"))) {
            if (arg_count == 4) {
                command[3] = '\0';
                writeSocket(sockfd, command);
                writeSocket(sockfd, command_arguments[1]);  // reservation date
                writeSocket(sockfd, command_arguments[2]);  // reserved room
                writeSocket(sockfd, command_arguments[3]);  // reservation code
            } else {
                printf("\x1b[33mInvalid command\x1b[0m:\n  missing arguments\n  release [date (mm/dd)] [room] [code]\n");
            }
        }

        else {
            /* command doesn't match anything the server is supposed 
             * to receive hence i don't send anything into the socket.
             */
            printf("\x1b[33mInvalid command.\x1b[0m \n");
        }
        

        printf("%s\n", response);

        #else

            memset(command, '\0', BUFSIZE);
            memset(response, '\0', BUFSIZE);


            switch (state)
            {
                case CL_INIT:
                // do stuff
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


                // update FSM
                    if      (strcmp(command, "help") == 0)  
                        state = SEND_HELP;
                    else if (strcmp(command, "login") == 0) 
                        state = SEND_LOGIN; 
                    else if (strcmp(command, "register") == 0) 
                        state = SEND_REGISTER;
                    // else if (strcmp(command, "view") == 0) 
                    //     state = SEND_VIEW;  
                    // else if (strcmp(command, "reserve") == 0) 
                    //      state = SEND_RESERVE;
                    else if (strcmp(command, "quit") == 0)
                        state = SEND_QUIT;
                    else
                        state = INVALID_UNLOGGED;
                    break;
    


                case INVALID_UNLOGGED:
                // do stuff
                    /* command doesn't match anything the server is supposed 
                     * to receive hence i don't send anything into the socket.
                     */
                    printf("\x1b[33mInvalid command.\x1b[0m \n");
                
                // update FSM
                    state = CL_INIT;
                    break;
                

                case SEND_HELP:
                // do stuff
                    writeSocket(sockfd, "h");   // trims the command to the first char only
                                                // since it uniquely indentifies the command. 
                                                // there's not need to ship the whole string
                
                // update FSM
                    state = READ_HELP_RESP;
                    break;

                case READ_HELP_RESP:
                // do stuff
                    readSocket(sockfd, response);
                    if (strcmp(response, "H") == 0){
                        printf("%s\n", HELP_UNLOGGED_MESSAGE);    
                    }
                // update FSM
                    state = CL_INIT;
                    break;

                case SEND_HELP_LOGGED:
                // do stuff
                    writeSocket(sockfd, "hh");
                
                // update FSM
                    state = READ_HELP_LOGGED_RESP;
                    break;

                case READ_HELP_LOGGED_RESP:
                // do stuff
                    readSocket(sockfd, response);
                    if (strcmp(response, "H") == 0){
                        printf("%s\n", HELP_LOGGED_IN_MESSAGE);    
                    }
                // update FSM
                    state = CL_LOGIN;
                    break;



                case SEND_QUIT:
                // do stuff
                    writeSocket(sockfd, "q");
                    printf( "You quit the system.\n");
                    goto ABORT;

                // update FSM
                    state = CL_INIT;   // you sure ?

                case SEND_REGISTER:
                // do stuff
                    writeSocket(sockfd, "r");
                
                // update FSM
                    state = READ_REGISTER_RESP;
                    break;

                case READ_REGISTER_RESP:
                // do stuff
                    readSocket(sockfd, command);    // Choose username
                    printf("%s", command);
                
                // update FSM
                    state = SEND_USERNAME;
                    break;

                case SEND_USERNAME:
                // do stuff
                    printf("> ");
                    fgets(username, 20, stdin);          // `\n` is included in username thus I replace it with `\0`
                    username[strlen(username)-1] = '\0';
                    
                    writeSocket(sockfd, username); 

                    // to be used later when i'm logged in and display the username
                    strcpy(user->username, username); 

                    
                // update FSM
                    state = READ_USERNAME_RESP;
                    break;

                case READ_USERNAME_RESP:
                // do stuff
                    readSocket(sockfd, command);
                    if (strcmp(command, "Y") == 0){
                        printf("%s\n", "username OK.");
                    }
                    else {
                        printf("%s\n", "\x1b[31m\033[1musername already taken.\x1b[0m\npick another one: ");
                        //                ^--red  ^--bold
                    }


                // update FSM
                    if (strcmp(command, "Y") == 0){
                        state = SEND_PASSWORD;
                    }
                    else {
                        state = SEND_USERNAME;
                    }
                    break;

                case SEND_PASSWORD:
                // do stuff
                    password = getpass("Choose password: ");

                    writeSocket(sockfd, password);
                
                // update FSM
                    rv = checkPasswordValidity(); // 0 is OK, -1 invalid
                    if (rv == 0){
                        state = READ_PASSWORD_RESP;
                    }
                    else {
                        state = SEND_PASSWORD;
                    }
                    break;
                
                case READ_PASSWORD_RESP:
                // do stuff
                    readSocket(sockfd, command);  // OK: Account was successfully setup.
                    printf("%s\n", command);
                    // memset(command, '\0', BUFSIZE);

                    readSocket(sockfd, command);  // Successfully registerd, you are now logged in
                    printf("%s\n", command);
                    // memset(command, '\0', BUFSIZE);

                // update FSM
                    state = CL_LOGIN;
                    break;

                case SEND_LOGIN:
                // do stuff
                    writeSocket(sockfd, "l");         
                // update FSM
                    state = READ_LOGIN_RESP;
                    break;

                case READ_LOGIN_RESP:
                // do stuff
                    readSocket(sockfd, command);
                    printf("%s\n", command);
                // update FSM
                    state = SEND_LOGIN_USERNAME;
                    break;

                case SEND_LOGIN_USERNAME:
                // do stuff
                    printf(": ");
                    fgets(username, 20, stdin);          // `\n` is included in username thus I replace it with `\0`
                    username[strlen(username)-1] = '\0';
                    writeSocket(sockfd, username);

                    // to be used later when i'm logged in and display the username
                    strcpy(user->username, username); 
                    
                // update FSM
                    state = READ_LOGIN_USERNAME_RESP;
                    break;

                case READ_LOGIN_USERNAME_RESP:
                // do stuff
                    readSocket(sockfd, command);
                    if (strcmp(command, "Y") == 0){
                        printf("%s\n", "OK.");
                        state = SEND_LOGIN_PASSWORD;
                    }
                    else {
                        printf("%s\n", "\x1b[31m\033[1musername unknown.\x1b[0m\nGo ahead and register first.");
                        //                ^--red  ^--bold
                        state = CL_INIT;
                    }
                    
                // update FSM
                    
                    break;

                case SEND_LOGIN_PASSWORD:
                // do stuff
                    password = getpass("Insert password: ");

                    writeSocket(sockfd, password);
                // update FSM
                    state = READ_LOGIN_PASSWORD_RESP;
                    break;

                case READ_LOGIN_PASSWORD_RESP:
                // do stuff
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
                // update FSM
                    
                    break;

                case CL_LOGIN:
                // do stuff
                    printf("\033[92m(%s)\x1b[0m> ", user->username);
                    
                    // read input
                    fgets(command, 20, stdin);

                    // drop the new line and replace w/ the string termination
                    command[strlen(command)-1] = '\0';
                    
                    // convert to input to lower case (for easier later comparison)
                    for (char *p = command; *p; ++p){
                        *p = *p >= 0x41 && *p <= 0x5A ? (*p | 0x60) : *p;
                    }

                // update FSM
                    if      (strcmp(command, "help") == 0)  
                        state = SEND_HELP_LOGGED;
                    // else if (strcmp(command, "view") == 0) 
                    //     state = SEND_VIEW;  
                    // else if (strcmp(command, "reserve") == 0) 
                    //      state = SEND_RESERVE;
                    else if (strcmp(command, "quit") == 0)
                        state = SEND_QUIT;
                    else if (strcmp(command, "logout") == 0)
                        state = SEND_LOGOUT;
                    else
                        state = INVALID_LOGGED_IN;
                    break;

                case INVALID_LOGGED_IN:
                    printf("\x1b[33mInvalid command.\x1b[0m \n");
                    state = CL_LOGIN;
                    
                    break;


                case SEND_LOGOUT:
                // do stuff
                    writeSocket(sockfd, "logout");
                // update FSM
                    state = CL_INIT;

                    // to be used later when i'm logged in and display the username
                    memset(user, '\0', sizeof(User));

                    break;

                default:
                    state = CL_INIT;

                
            }
            

            printClientFSMState(&state);



        #endif

    }

    free(username);
    free(password);



ABORT:
    close(sockfd);

    return 0;
}




/* . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . */





int checkIfUserAlreadyLoggedIn(){
    /*  */
    return 0;
}


int checkPasswordValidity(){
    /*  */
    return 0;
}