/**
 * @project:        hotel-booking
 * @file:           User.h
 * @author(s):      Francesco Urbani <https://urbanij.github.io/>
 *
 * @date:           Sat Jun 29 17:04:46 CEST 2019
 * @Description:    represents user "object"
 *
 */


#ifndef USER_H
#define USER_H

#include "Booking.h"


typedef struct user {
    char        username[20];
    char        actual_password[20];
    Booking     booking[MAX_BOOKINGS_PER_USER]; // array to keep track of user's bookings
    int         active_bookings;                // how many active bookings he has
} User;






/********************************/




#endif
