/**
 * @name            hotel-booking
 * @file            User.h
 * @author          Francesco Urbani <https://urbanij.github.io/>
 *
 * @date            Sat Jun 29 17:04:46 CEST 2019
 * @brief           User
 *
 */

#ifndef USER_H
#define USER_H

#include "config.h"

typedef struct user {
    char        username[USERNAME_MAX_LENGTH];
    char        actual_password[PASSWORD_MAX_LENGTH];
} User;


#endif
