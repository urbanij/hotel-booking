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


#define DEBUG               1





#define NUM_THREADS             2           // # threads
#define NUM_CONNECTION          10          // # queued connections
#define BUFSIZE                 512         // buffer size: maximum length of messages
#define BACKLOG                 10          // listen() function parameter

#define MAX_BOOKINGS_PER_USER   5           // max number of bookings allowed for each user




#endif


