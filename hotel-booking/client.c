/**
 * @project:        hotel-booking
 * @file:           client.c
 * @author(s):      Francesco Urbani <https://urbanij.github.io/>
 *
 * @date:           Mon Jul  1 12:43:37 CEST 2019
 * @Description:    client side, main
 *
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

// config definition and declarations
#include "config.h"

#include "utils.h"

#include "Address.h"
#include "Hotel.h"
#include "User.h"


/********************************/
/*                              */
/*          global var          */
/*                              */
/********************************/


/********************************/
/*                              */
/*          protoypes           */
/*                              */
/********************************/


// FSM related 
fsm_state_t* updateClientFSM       (fsm_state_t* state, char* command);

int closeConnection(int);


/********************************/
/*                              */
/*          functions           */
/*                              */
/********************************/




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
    while (1) 
    {
        
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

        

    }







    // close the socket
    close(sockfd);

    return 0;
}



fsm_state_t* updateClientFSM(fsm_state_t* state, char* command)
{
    int rv; 
    
    switch (*state)
    {

        case INIT:
            if      (strcmp(command, "h") == 0)  // help
                *state = HELP_UNLOGGED;
            else if (strcmp(command, "v") == 0)  // view
                *state = INIT;
            else if (strcmp(command, "r") == 0)  // register
                *state = CHECK_IF_LOGGED_IN;
            else if (strcmp(command, "l") == 0)  // login
                *state = CHECK_USERNAME;
            else if (strcmp(command, "q") == 0)  // quit
                *state = QUIT;
            else
                *state = INIT;
            break;
            
        case HELP_UNLOGGED:
            *state = INIT;
            break;

        case CHECK_USERNAME:
            rv = 0;
            if (rv == 0){
                *state = CHECK_PASSWORD;
            }
            else {
                *state = INIT;   
            }
            break;

        case CHECK_PASSWORD:
            rv = 0;
            if (rv == 0){
                *state = LOGIN;
            }
            else {
                *state = INIT;   
            }
            break;

        case CHECK_IF_LOGGED_IN:
            rv = 0;
            if (rv == 0){
                *state = REGISTER;
            }
            else {
                *state = INIT;
            }
            break;
        
        case REGISTER:
            *state = PICK_USERNAME;
            break;
        
        case PICK_USERNAME:
            *state = PICK_PASSWORD;
            break;
        
        case PICK_PASSWORD:
            rv = 0;
            if (rv == 0){
                *state = LOGIN;
            }
            else {
                *state = PICK_USERNAME;
            }
            break;

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
            rv = 0;
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
            rv = 0;
            if (rv == 0){
                *state = RELEASE;
            }
            else {
                *state = LOGIN;
            }
            break;

        case RELEASE:
            *state = LOGIN;
            break;

        case VIEW:
            *state = LOGIN;
            break;

        case LOGOUT:
            *state = INIT;
            break;

        case QUIT:
            *state = INIT;  // makes no sense tho, it's quitting anyway...
            break;

        default:
            *state = INIT;
    }
    
    return  state;
}






int closeConnection(int sockfd)
{
    writeSocket(sockfd, "q");
    close(sockfd);
    printf( "You quit the system.\n");
    return 0;
}


