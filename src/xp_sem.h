/**
 * Francesco Urbani
 * Fri May 24 19:34:46 CEST 2019
 *
 *
 * `semaphore.h` is deprecated on macOS, hence this header file.
 * (https://stackoverflow.com/a/27847103)
 */



#ifndef XP_SEM  // XP for cross-platform, btw
#define XP_SEM




#include <errno.h>
#include <stdint.h>


#ifdef __APPLE__
    #include <dispatch/dispatch.h>
#else
    #include <semaphore.h>
#endif





typedef struct xp_sem {
    #ifdef __APPLE__
        dispatch_semaphore_t    sem;
    #else
        sem_t                   sem;
    #endif
} xp_sem_t;



/**
 *    sem_init
 */
static inline void
xp_sem_init(xp_sem_t* s, int pshared, uint32_t value)
{
    #ifdef __APPLE__
        dispatch_semaphore_t *sem = &s->sem;
        *sem = dispatch_semaphore_create(value);
    #else
        sem_init(&s->sem, pshared, value);
    #endif
}



/**
 *    sem_wait
 */
static inline void 
xp_sem_wait(xp_sem_t* s)
{
    #ifdef __APPLE__
        dispatch_semaphore_wait(s->sem, DISPATCH_TIME_FOREVER);
    #else
        sem_wait(&s->sem);
    #endif
}





/**
 *    sem_post
 */
static inline void
xp_sem_post(xp_sem_t* s)
{
    #ifdef __APPLE__
        dispatch_semaphore_signal(s->sem);
    #else
        sem_post(&s->sem);
    #endif
}


// aliases 'cause Dijkstra was a badass.
#define proberen   xp_sem_wait
#define verhogen   xp_sem_post


#endif
