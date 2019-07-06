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


#define DEBUG                   1




#define NUM_THREADS             2           // # threads
#define NUM_CONNECTION          10          // # queued connections
#define BUFSIZE                 512         // buffer size: maximum length of messages
#define BACKLOG                 10          // listen() function parameter

#define MAX_BOOKINGS_PER_USER   5           // max number of bookings allowed for each user

#define USER_FILE               "users.txt" // text file containig users and encrypted passwords

#define DATABASE                "bookings.db"




// fancy options
#define HELP_MESSAGE_TYPE_1     0       // if 1 uses type 1, else type2

#define ENCRYP_PASSWORD         1       // has to be 1 in production





#endif

