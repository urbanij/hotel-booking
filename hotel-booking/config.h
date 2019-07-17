/**
 * @name            hotel-booking
 * @file            config.h
 * @author          Francesco Urbani <https://urbanij.github.io/>
 *
 * @date            Mon Jul  1 12:43:37 CEST 2019
 * @brief           configuration file. edit this before compiling.
 *
 */


#ifndef CONFIG_H
#define CONFIG_H


////////////////////////// customizable //////////////////////////


#define GDB_MODE                0       // has to be 0 in production
                                        // if not specified the compilation flag
                                        // -DGDB_MODE is required for it to be active.





#define NUM_THREADS             2       // # threads
#define NUM_CONNECTION          10      // # queued connections

#define MAX_BOOKINGS_PER_USER   5       // max number of bookings allowed for each user

#define PASSWORD_MAX_LENGTH     32
#define PASSWORD_MIN_LENGTH     4
#define USERNAME_MAX_LENGTH     16
#define USERNAME_MIN_LENGTH     2



#define DEBUG                   1       // debug mode: prints messages to the console
#if DEBUG
    #define VERBOSE_DEBUG       1       // even more debug messages
    #if VERBOSE_DEBUG
    #define VERY_VERBOSE_DEBUG  0
    #endif
#endif

#define HELP_MESSAGE_TYPE_1     0       // 1 for type 1, 0 for type 2
#define SORT_VIEW_BY_DATE       1       // sort view response by date rather than by order of reservation


#define HIDE_PASSWORD           1       // whether hiding or not the password when the user types it in.




////////////////////////////// data //////////////////////////////


#define DATA_FOLDER             ".data"

// USER_FILE and DATABASE will be saved inside DATA_FOLDER/
#define USER_FILE_NAME          "users.txt"         ///< text file containig users and relative encrypted passwords
#define DATABASE_NAME           "bookings.db"



////////////////////////// miscellaneous //////////////////////////

#define BUFSIZE                 2048    // buffer size: maximum length of messages
#define BACKLOG                 10      // listen() function parameter



////////////////////////// design directives //////////////////////////

#define ENCRYPT_PASSWORD        1
#define RESERVATION_CODE_LENGTH 6       // 5 + '\0'     // carefeul, if you change this the regex `REGEX_CODE` has to be manually changed.




////////////////////////// fancy options //////////////////////////




// ANSI terminal colors
#define ANSI_COLOR_RED          "\x1b[31m"
#define ANSI_COLOR_GREEN        "\x1b[32m"
#define ANSI_COLOR_YELLOW       "\x1b[33m"
#define ANSI_COLOR_BLUE         "\x1b[34m"
#define ANSI_COLOR_MAGENTA      "\x1b[35m"
#define ANSI_COLOR_BMAGENTA     "\x1b[95m"
#define ANSI_COLOR_CYAN         "\x1b[36m"
#define ANSI_COLOR_RESET        "\x1b[0m"
#define ANSI_BOLD               "\033[1m"


#endif

