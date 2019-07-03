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

static client_fsm_state_t  state;

/********************************/
/*                              */
/*          protoypes           */
/*                              */
/********************************/


// FSM related
/**
 *
 */ 
client_fsm_state_t* updateClientFSM       (client_fsm_state_t* state, char* command);

// int closeConnection(int);


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
int usernameIsOk();

/**
 *
 */
int checkPasswordValidity();





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

    char username[20];
    char password[30];

    #define LOCK_INFINITE_CYCLE 0
    #if LOCK_INFINITE_CYCLE
        int temporary_cycle_counter = 0;
    #endif



    state = CL_INIT;

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
                    printf("> ");
                    #if 0
                        scanf("%[^\n]%*c", command);    // https://stackoverflow.com/a/6282236/6164816
                                                        // while the input is not a newline ('\n') take input.
                                                        // Then with the %*c it reads the newline character
                                                        // from the input buffer (which is not read), and the *
                                                        // indicates that this read in input is discarded
                    #else
                        fgets(command, 20, stdin);          // `\n` is included in command thus I
                        command[strlen(command)-1] = '\0';  // replace it with the string termination char
                    #endif
                    
                    break;



                #if 1
                case INVALID:
                    /* command doesn't match anything the server is supposed 
                     * to receive hence i don't send anything into the socket.
                     */
                    printf("\x1b[33mInvalid command.\x1b[0m \n");
                    break;
                #endif


                case SEND_HELP:
                    writeSocket(sockfd, "h");   // trims the command to the first char only
                                                // since it uniquely indentifies the command. 
                                                // there's not need to ship the whole string
                    break;

                case SEND_HELP_LOGGED:
                    writeSocket(sockfd, "hh");
                    break;

                case READ_HELP_RESP:
                    readSocket(sockfd, response);
                    printf("%s\n", response);
                    break;

                case SEND_QUIT:
                    writeSocket(sockfd, "q");
                    printf( "You quit the system.\n");
                    goto ABORT;

                case SEND_REGISTER:
                    writeSocket(sockfd, "r");
                    break;

                case READ_REGISTER_RESP:
                    readSocket(sockfd, command);    // Choose username
                    printf("%s", command);
                    break;

                case SEND_USERNAME:
                    printf("> ");
                    fgets(username, 20, stdin);          // `\n` is included in username thus I replace it with `\0`
                    username[strlen(username)-1] = '\0';
                    
                    writeSocket(sockfd, username); 
                    memset(username, '\0', sizeof(username));
                    break;

                case READ_USERNAME_RESP:
                    readSocket(sockfd, command);
                    printf("%s\n", command);          // should print `username OK`
                    break;

                case SEND_PASSWORD:
                    printf("> ");
                    fgets(username, 20, stdin);          // `\n` is included in username thus I replace it with `\0`
                    username[strlen(username)-1] = '\0';

                    writeSocket(sockfd, password);
                    memset(password, '\0', sizeof(password));
                    break;
                
                case READ_PASSWORD_RESP:
                    readSocket(sockfd, command);  // OK: Account was successfully setup.
                    printf("%s\n", command);
                    // memset(command, '\0', BUFSIZE);

                    readSocket(sockfd, command);  // Successfully registerd, you are now logged in
                    printf("%s\n", command);
                    // memset(command, '\0', BUFSIZE);

                    break;

                case CL_LOGIN:
                    printf("\033[92m(logged-in)\x1b[0m> ");
                    
                    fgets(command, 20, stdin);          // `\n` is included in command thus I
                    command[strlen(command)-1] = '\0';  // replace it with the string termination char
                    break;

                
            }



            updateClientFSM(&state, command);

            printClientFSMState(&state);
            // printf("%s", printClientFSMState(state));




        #endif

    }





ABORT:
    close(sockfd);

    return 0;
}





client_fsm_state_t* updateClientFSM(client_fsm_state_t* state, char* command)
{
    int rv; 
    
    switch (*state)
    {

        case CL_INIT:
            if      (strcmp(command, "help") == 0)  
                *state = SEND_HELP;
            else if (strcmp(command, "login") == 0) 
                *state = SEND_USERNAME; 
            else if (strcmp(command, "register") == 0) 
                *state = SEND_REGISTER;
            // else if (strcmp(command, "view") == 0) 
            //     *state = SEND_VIEW;  
            // else if (strcmp(command, "reserve") == 0) 
            //      *state = SEND_RESERVE;
            else if (strcmp(command, "quit") == 0)
                *state = SEND_QUIT;
            else
                *state = INVALID;
            break;
    
        
        case INVALID:
            *state = CL_INIT;
            break;


        case SEND_HELP:
            *state = READ_HELP_RESP;
            break;

        case SEND_HELP_LOGGED:
            *state = READ_HELP_RESP;
            break;
        
        case READ_HELP_RESP:
            *state = CL_INIT;
            break;

        case SEND_QUIT:
            *state = CL_INIT;   // you sure ?
            break;

        case SEND_REGISTER:
            *state = READ_REGISTER_RESP;
            break;

        case READ_REGISTER_RESP:
            *state = SEND_USERNAME;
            break;

        case SEND_USERNAME:
            *state = READ_USERNAME_RESP;
            break;

        case READ_USERNAME_RESP:
            rv = usernameIsOk();   // 0 if Ok, -1 if altready used.
            if (rv == 0){
                *state = SEND_PASSWORD;
            }
            else {
                *state = SEND_USERNAME;
            }
            break;

        case SEND_PASSWORD:
            rv = checkPasswordValidity(); // 0 is OK, -1 invalid
            if (rv == 0){
                *state = READ_PASSWORD_RESP;
            }
            else {
                *state = SEND_PASSWORD;
            }
            break;

        case READ_PASSWORD_RESP:
            *state = CL_LOGIN;

        
        case CL_LOGIN:
            if      (strcmp(command, "help") == 0)  
                *state = SEND_HELP_LOGGED;
            // else if (strcmp(command, "view") == 0) 
            //     *state = SEND_VIEW;  
            // else if (strcmp(command, "reserve") == 0) 
            //      *state = SEND_RESERVE;
            else if (strcmp(command, "quit") == 0)
                *state = SEND_QUIT;
            // else
            //     *state = INVALID;
            break;



        default:
            *state = CL_INIT;
    }
    
    return  state;
}








int checkIfUserAlreadyLoggedIn(){
    /*  */
    return 0;
}

int usernameIsOk(){
    /*  */
    return 0;
}

int checkPasswordValidity(){
    /*  */
    return 0;
}