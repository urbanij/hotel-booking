/**
 * @project:        hotel-booking
 * @file:           Address.h
 * @author(s):      Francesco Urbani <https://urbanij.github.io/>
 *
 * @date:           Mon Jul  1 12:43:37 CEST 2019
 * @Description:    represents ip and port
 *
 */

#ifndef ADDRESS_H
#define ADDRESS_H



typedef struct address {
    #if 0
        char*   ip;
    #else
        char    ip[16];
    #endif
    int     port;
} Address;


void repr_addr(Address* a);    // "member function of Address class"



/********************************/


void
repr_addr(Address* addr)
{
    printf("%s:%d\n", addr->ip, addr->port);
}


#endif
