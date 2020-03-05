/**
 * Rendezvous
 *
 * Two threads executing chunks of work in lock step.
 *
 * Author: Nikos Nikoleris <nikos.nikoleris@it.uu.se>
 *
 */

#include <stdio.h>   /* printf() */
#include <stdlib.h>  /* abort(), [s]rand() */
#include <unistd.h>  /* sleep() */
#include <pthread.h> /* pthread_...() */

#include "psem.h"

#define LOOPS 10
#define NTHREADS 3
#define MAX_SLEEP_TIME 3

// Declare global semaphore variables. Note, they must be initialized before use.
psem_t *semA, *semB;

/* TODO: Make the two threads perform their iterations in lockstep. */

void *
threadA(void *param __attribute__((unused)))
{
    int i;

    for (i = 0; i < LOOPS; i++)
    {
        psem_wait(semB);
        printf("A%d\n", i);
        sleep(rand() % MAX_SLEEP_TIME);
        psem_signal(semA);
    }

    pthread_exit(0);
}

/* TODO: Make the two threads perform their iterations in lockstep. */

void *
threadB(void *param __attribute__((unused)))
{
    int i;

    for (i = 0; i < LOOPS; i++)
    {
        psem_wait(semA);
        printf("B%d\n", i);
        sleep(rand() % MAX_SLEEP_TIME);
        psem_signal(semB);
    }

    pthread_exit(0);
}

int main()
{
    pthread_t tidA, tidB;

    // Todo: Initialize semaphores.

    semA = psem_init(1);
    semB = psem_init(1);

    srand(time(NULL));
    pthread_setconcurrency(3);

    if (pthread_create(&tidA, NULL, threadA, NULL) ||
        pthread_create(&tidB, NULL, threadB, NULL))
    {
        perror("pthread_create");
        abort();
    }
    if (pthread_join(tidA, NULL) != 0 ||
        pthread_join(tidB, NULL) != 0)
    {
        perror("pthread_join");
        abort();
    }

    // Todo: Destroy semaphores.
    psem_destroy(semA);
    psem_destroy(semB);

    return 0;
}
/**
 * 
 * Q: Explain the concept of rendezvous.
 * Rendevous is a "meeting point", here we want two threads to have a meeting point after each iteration
 * 
 * Q: What will happen when you wait on a semaphore?
 * It will check that it's > 0, if not it will wait for it to be, otherwise it will decrement it
 * 
 * Q: What will happen when you signal on a semaphore?
 * It increments the semaphore
 * 
 * Q: How can semaphores be used to enforce rendezvous between two threds?
 * Well, like above, we create two binary semaphores and each thread have to wait for the semaphore that
 * the other thread can signal to. I.e, if it's >0 the other process have signaled to it and the thread can print
 * for that iteration and then signal its own semaphore
 * 
 * Q: How are mutex locks different compared to semaphores?
 * The semaphore is a signaling mechanism which use wait() and signal() to 
 * indicate whether they are acquring or releasing the resource,
 * while a mutex requires the thread to acquire the lock itself and release it after it's "done" with the shared resource
 * 
 * Q: Why can’t mutex locks be used to solve the rendezvous problem?
 * A mutex lock is meant to be taken and released, 
 * always in that order, by each task that uses the shared resource it protects.
 * 
 * Therefore, we generally don't want to use mutex locks for something like this, 
 * since we want to use a semaphore to signal between tasks
 * I.e. a task either performs wait or signal. Not both.
*/