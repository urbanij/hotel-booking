/**
 * @name            hotel-booking
 * @file            messages.h
 * @author          Francesco Urbani <https://urbanij.github.io/>
 *
 * @date            Thu Jul 11 09:23:06 CEST 2019
 * @brief           messages that the client prints to the console are grouped here
 *
 */


#ifndef MESSAGES_H
#define MESSAGES_H

#include "config.h"




#if HELP_MESSAGE_TYPE_1

    // Only show the allowed options

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

    // show ALL the options

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

#define INVALID_COMMAND_MESSAGE         "\x1b[31mInvalid command.\x1b[0m\n"
#define UNREGISTERED_USERNAME_ERR_MSG   "\x1b[31mUnregistered username.\x1b[0m\nGo ahead and register first.\n"
#define INVALID_DATE_MSG                "\x1b[31mInvalid date.\x1b[0m Make sure the day actually exists in 2020.\n"
#define INVALID_FORMAT_RESERVE_MSG      "\x1b[31mInvalid format.\x1b[0m Make sure the foramt is:\n\t        reserve [dd/mm]\n"
#define INVALID_FORMAT_RELEASE_MSG      "\x1b[31mInvalid format.\x1b[0m Make sure the format is:\n\t        release [dd/mm] [room] [code]\n"
#define WRONG_PASSWORD_MSG              "\x1b[31m\033[1mwrong password.\x1b[0m Try to login again...\n"
#define ACCESS_GRANTED_MSG              "OK, access granted.\n"
#define USERNAME_TAKEN_MSG              "\x1b[31m\033[1musername already taken.\x1b[0m\n"
#define USERNAME_PROMPT_MSG             "Insert username: "
#define PASSWORD_PROMPT_MSG             "Insert password: "




// the following definitions have to be unique.

#define HELP_MSG                        "h"
#define REGISTER_MSG                    "r"
#define LOGIN_MSG                       "l"
#define QUIT_MSG                        "q"
#define LOGOUT_MSG                      "lgt"
#define VIEW_MSG                        "v"
#define RESERVE_MSG                     "res"
#define RELEASE_MSG                     "rel"




#endif




