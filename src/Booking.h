/**
 * @name            hotel-booking
 * @file            Booking.h
 * @author          Francesco Urbani <https://urbanij.github.io/>
 *
 * @date            Mon Jul  1 12:43:37 CEST 2019
 * @brief           stores booking data
 *
 */

#ifndef BOOKING_H
#define BOOKING_H

typedef struct booking {
    char    date[6];    // date is at most 6 since the date the user
                        // is supposed to insert is - at most- 5 chars long
                        // 24/10 (Oct 24).
    char    room[4];    
    char    code[RESERVATION_CODE_LENGTH];    // alphanumeric and autogenerated
} Booking;




void printBooking(Booking* b){
    printf("Booking on date %s, in room %s, with code %s\n", 
            b->date, b->room, b->code
        );
    return;
}




#endif
