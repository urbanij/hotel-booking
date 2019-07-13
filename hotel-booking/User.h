/**
 * @name            hotel-booking <https://github.com/urbanij/hotel-booking>
 * @file            User.h
 * @author          Francesco Urbani <https://urbanij.github.io/>
 *
 * @date            Sat Jun 29 17:04:46 CEST 2019
 * @brief           represents user "object"
 *
 */


#ifndef USER_H
#define USER_H

#include "Booking.h"


typedef struct user {
    char        username[USERNAME_MAX_LENGTH];
    char        actual_password[PASSWORD_MAX_LENGTH];
    Booking     booking[MAX_BOOKINGS_PER_USER]; // array to keep track of user's bookings
    int         active_bookings;                // how many active bookings he has
} User;




#endif
