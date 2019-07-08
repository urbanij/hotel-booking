/**
 * @project:        hotel-booking
 * @file:           config.h
 * @author(s):      Francesco Urbani <https://urbanij.github.io/>
 *
 * @date:           Mon Jul  1 12:43:37 CEST 2019
 * @Description:    
 *
 */


#ifndef CONFIG_H
#define CONFIG_H



#define NUM_THREADS             2           // # threads
#define NUM_CONNECTION          10          // # queued connections
#define BUFSIZE                 2048        // buffer size: maximum length of messages
#define BACKLOG                 10          // listen() function parameter

#define MAX_BOOKINGS_PER_USER   5           // max number of bookings allowed for each user

#define USER_FILE               "users.txt" // text file containig users and encrypted passwords

#define DATABASE                "bookings.db"






#define PASSWORD_MAX_LENGTH     10



// fancy options
#define DEBUG                   1

#define HELP_MESSAGE_TYPE_1     1       // 1 for type 1, 2 for type 2

#define ENCRYPT_PASSWORD        1       // has to be 1 in production




// ANSI terminal colors
#define ANSI_COLOR_RED          "\x1b[31m"
#define ANSI_COLOR_GREEN        "\x1b[32m"
#define ANSI_COLOR_YELLOW       "\x1b[33m"
#define ANSI_COLOR_BLUE         "\x1b[34m"
#define ANSI_COLOR_MAGENTA      "\x1b[35m"
#define ANSI_COLOR_BMAGENTA     "\x1b[95m"
#define ANSI_COLOR_CYAN         "\x1b[36m"
#define ANSI_COLOR_RESET        "\x1b[0m"


#endif

