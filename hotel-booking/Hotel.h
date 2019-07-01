/**
 * @project:        hotel-booking
 * @file:           Hotel.h
 * @author(s):      Francesco Urbani <https://urbanij.github.io/>
 *
 * @date:           Mon Jul  1 12:43:37 CEST 2019
 * @Description:    represents hotel "object"
 *
 */

#ifndef HOTEL_H
#define HOTEL_H


// class
typedef struct hotel {
    int     total_rooms;
    int     booked_rooms[12][31];   // rooms available for each day of the year
} Hotel;


void initializeHotel(Hotel* h);

// methods declaration
int bookRoom(Hotel* h, int day, int month);







// methods definitions
void initializeHotel(Hotel* h){
    for (int i = 0; i < 12; i++) {
        for (int j = 0; j < 31; j++) {
            h->booked_rooms[i][j] = 0;
        }
    }
    return;
}


/**
 * return 0 is booking is successful, otherwise -1
 */
int bookRoom(Hotel* h, int day, int month){
    if (h->booked_rooms[day][month] < h->total_rooms){
        h->booked_rooms[day][month]++;
        return 0;   // success
    }
    else {
        return -1;  // failure
    }
}



#endif
