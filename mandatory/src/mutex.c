/**
 * Critical sections and mutual exclusion.
 *
 * Multiple threads incrementing and decrementing the same shared variable.
 *
 * History:
 *
 * 2013 - Original version by Nikos Nikoleris <nikos.nikoleris@it.uu.se>.
 *
 * 2019 - Refactor and added stats summary by Karl Marklund <karl.marklund@it.uu.se>.
 */

#include <stdio.h>   // printf(), fprintf()
#include <stdlib.h>  // abort()
#include <pthread.h> // pthread_...
#include <stdbool.h> // true, false

#include "timing.h" // timing_start(), timing_stop()

/* Shared variable */
volatile int counter;

/* Pthread mutex lock */
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/* Shared variable used to implement a spinlock */
volatile int lock = false;

/* Number of threads that will increment the shared variable */
#define INC_THREADS 5
/* Value by which the threads increment the shared variable */
#define INCREMENT 2
/* Iterations performed incrementing the shared variable */
#define INC_ITERATIONS 20000
/* Number of threads that will try to decrement the shared variable */
#define DEC_THREADS 4
/* Value by which the threads increment the shared variable */
#define DECREMENT 2
/* Iterations performed decrementing the shared variable.*/
#define DEC_ITERATIONS (INC_ITERATIONS * INC_THREADS * INCREMENT / DEC_THREADS / DECREMENT)

/*******************************************************************************
                          Test 0 - No synchronization
*******************************************************************************/

/* Unsynchronized increments of the shared counter variable */
void *
inc_no_sync(void *arg __attribute__((unused)))
{
    int i;

    for (i = 0; i < INC_ITERATIONS; i++)
    {
        counter += INCREMENT;
    }

    return NULL;
}

/* Unsynchronized decrements of the shared counter variable */
void *
dec_no_sync(void *arg __attribute__((unused)))
{
    int i;

    for (i = 0; i < DEC_ITERATIONS; i++)
    {
        counter -= DECREMENT;
    }

    return NULL;
}

/*******************************************************************************
                          Test 1 - Pthread mutex lock
*******************************************************************************/

/* Increments of the shared counter should be protected by a mutex */
void *
inc_mutex(void *arg __attribute__((unused)))
{
    int i;

    for (i = 0; i < INC_ITERATIONS; i++)
    {
        /* TODO: Protect access to the shared variable counter with a mutex lock
         * inside the loop. */
        /* from documentation:
        The pthread_mutex_lock() routine is used by a thread to acquire a lock on the specified mutex variable. 
        If the mutex is already locked by another thread, this call will block the calling thread until the mutex is unlocked.
        */
        pthread_mutex_lock(&mutex); // Locks
        counter += INCREMENT; // Critial section
        pthread_mutex_unlock(&mutex); // Unlocks
    }

    return NULL;
}

/* Decrements of the shared counter should be protected by a mutex */
void *
dec_mutex(void *arg __attribute__((unused)))
{
    int i;

    for (i = 0; i < DEC_ITERATIONS; i++)
    {
        /* TODO: Protect access to the shared variable counter with a mutex lock
         * inside the loop. */
        pthread_mutex_lock(&mutex); // Locks
        counter -= DECREMENT; // Critical section
        pthread_mutex_unlock(&mutex); // Unlocks
    }

    return NULL;
}

/*******************************************************************************
                      Test 2 - Spinlock with test-and-set
*******************************************************************************/

void spin_lock() {
    // The calling operation obtains the lock if false, otherwise the while-loop "spins" waiting to acquire the lock before entering the critical section
  while (__sync_lock_test_and_set(&lock, true)); // Sets to true and returns the previous value
}

void spin_unlock() {
  __sync_lock_release(&lock, false);
}

/* Increments of the shared counter should be protected by a test-and-set spinlock */
void *
inc_tas_spinlock(void *arg __attribute__((unused)))
{
  int i;

    for (i = 0; i < INC_ITERATIONS; i++)
    {
        /* TODO: Add the spin_lock() and spin_unlock() operations inside the loop. */
        spin_lock(); // While loop if locked (i.e. it returns the previous value which then is true)
        counter += INCREMENT; // Critial section
        spin_unlock(); // "Unlocks", releases, the lock by setting it to false
    }

  return NULL;
}

/* Decrements of the shared counter should be protected by a test-and-set spinlock */
void *
dec_tas_spinlock(void *arg __attribute__((unused)))
{
  int i;

    for (i = 0; i < DEC_ITERATIONS; i++)
    {
        /* TODO: Add the spin_lock() and spin_unlock() operations inside the loop. */
        spin_lock(); // The while loop which will continue if true
        counter -= DECREMENT;
        spin_unlock(); // Releases the lock
    }

  return NULL;
}

/*******************************************************************************
                      Tes 3 - Atomic addition/subtraction
*******************************************************************************/

/* Increment the shared counter using an atomic increment instruction */
// From docu: 
// These built-in functions perform the operation suggested by the name, and returns the value that had previously been in memory

void *
inc_atomic(void *arg __attribute__((unused)))
{
  int i;

  for (i = 0; i < INC_ITERATIONS; i++)
    {
      __sync_fetch_and_add(&counter, INCREMENT); // Atomic add
    }

  return NULL;
}

/* Decrement the shared counter using an atomic increment instruction */
void *
dec_atomic(void *arg __attribute__((unused)))
{
  int i;

  for (i = 0; i < DEC_ITERATIONS; i++)
    {
      __sync_fetch_and_sub(&counter, DECREMENT); // Atomic sub
    }

  return NULL;
}

/*******************************************************************************
 *******************************************************************************
            NOTE: You don't need to modify anything below this line
 *******************************************************************************
 ******************************************************************************/

/* Each test case is represented by the following struct. */

typedef struct
{
    char *name;           // Test case name.
    void *(*inc)(void *); // Increment function.
    void *(*dec)(void *); // Decrement function.
    double total_time;    // Total runtime;
    double average_time;  // Average execution time per thread.
    int counter;          // Final value of the shared counter.
} test_t;

test_t tests[] = {
    {.inc = inc_no_sync, .dec = dec_no_sync, .name = "No synchronization"},
    {.inc = inc_mutex, .dec = dec_mutex, .name = "Pthread mutex"},
    {.inc = inc_tas_spinlock, .dec = dec_tas_spinlock, .name = "Spinlock"},
    {.inc = inc_atomic, .dec = dec_atomic, .name = "Atomic add/sub"},
    {.inc = NULL, .dec = NULL, .name = NULL}};

// Information about each thread will be kept in the following struct.

typedef struct
{
    // Pthread ID (tid) of the created thread will be stored here after calling pthread_create().
    pthread_t tid;
    // Numeric thread ID.
    int id;
    // Type of thread (increment or decrement).
    enum type
    {
        inc,
        dec
    } type;
    // The created thread will start to execute in the start_routine function ...
    void *(*start_routine)(void *);
    // ... with arg as its sole argument.
    void *arg;
    // Total runtime of the thread.
    double run_time;
} thread_t;

char *type2string(enum type type)
{
    switch (type)
    {
    case inc:
        return "inc";
    case dec:
        return "dec";
    default:
        return "???";
    }
}

/* The startroutine used by both increment and decrement threads. */
void *
generic_thread(void *_conf)
{
    struct timespec ts;
    thread_t *conf = (thread_t *)_conf;

    timing_start(&ts);

    conf->start_routine(conf->arg);

    conf->run_time = timing_stop(&ts);

    pthread_exit(0);
}

double
print_stats(thread_t *threads, int nthreads, int niterations, test_t *test)
{
    double run_time_sum = 0;
    double average_execution_time = 0;

    printf("\nStatistics:\n\n");
    for (int i = 0; i < nthreads; i++)
    {
        thread_t *t = &threads[i];
        printf("Thread %i (%s): %.4f sec (%.4e iterations/s)\n",
               i, type2string(t->type), t->run_time,
               niterations / nthreads / t->run_time);
        run_time_sum += t->run_time;
    }

    average_execution_time = run_time_sum / nthreads;

    printf("\nAverage execution time: %.4f s/thread\n"
           "\nAvergage iterations/second: %.4e iterations/s\n",
           average_execution_time,
           niterations / nthreads / run_time_sum);

    test->total_time = run_time_sum;
    return average_execution_time;
}

char *successOrFailure(int counter)
{
    return (counter == 0) ? "success" : "failure";
}
void print_stats_summary(test_t tests[])
{
    test_t *test = tests;
    int width = 20;

    printf("\n\n=========================================================================================\n\n");
    printf("                                       SUMMARY\n\n\n");

    printf("%*s                             Total run      Average execution time\n", width, "");
    printf("%*s     Counter     Result      time (sec)     per thread (sec/thread)\n", width, "Test Case");
    printf("-----------------------------------------------------------------------------------------\n");

    while (test->inc && test->dec)
    {
        printf("%*s     %-10d  %s     %f       %f\n",
               width,
               test->name,
               test->counter,
               successOrFailure(test->counter),
               test->total_time,
               test->average_time);
        test++;
    }
}

void run_test(test_t *test)
{
    int i, nthreads = 0;
    thread_t threads[INC_THREADS + DEC_THREADS];
    double average_execution_time = 0;

    pthread_setconcurrency(INC_THREADS + DEC_THREADS);

    counter = 0;

    pthread_setconcurrency(INC_THREADS + DEC_THREADS + 1);

    /* Create the incrementing threads */

    for (i = 0; i < INC_THREADS; i++)
    {
        thread_t *thread = &threads[nthreads];
        thread->id = nthreads;
        thread->type = inc;
        thread->start_routine = test->inc;
        if (pthread_create(&thread->tid, NULL, generic_thread, thread) != 0)
        {
            perror("pthread_create");
            abort();
        }
        nthreads++;
    }

    /* Create the decrementing threads */

    for (i = 0; i < DEC_THREADS; i++)
    {
        thread_t *thread = &threads[nthreads];
        thread->id = nthreads;
        thread->type = dec;
        thread->start_routine = test->dec;
        if (pthread_create(&thread->tid, NULL, generic_thread, thread) != 0)
        {
            perror("pthread_create");
            abort();
        }
        nthreads++;
    }

    /* Wait for all threads to terminate */

    for (i = 0; i < nthreads; i++)
        if (pthread_join(threads[i].tid, NULL) != 0)
        {
            perror("pthread_join");
            abort();
        }
    printf("\n==========================================================================\n");
    printf("%s\n\n", test->name);
    printf("Counter expected value:%10d\n", 0);
    printf("Counter actual value:  %10d\n", counter);

    test->counter = counter;

    if (counter != 0)
    {
        printf("\nFAILURE :-(\n");
    }
    else
    {
        printf("\nSUCCES :-)\n");
    }

    average_execution_time = print_stats(threads, nthreads, INC_ITERATIONS + DEC_ITERATIONS, test);
    test->average_time = average_execution_time;
}

int main()
{
    test_t *test = tests;

    while (test->inc && test->dec)
    {
        run_test(test);
        test++;
    }

    print_stats_summary(tests);

    exit(EXIT_SUCCESS);
}

/***
 * Explain the following concepts and relate them to the source code and the behaviour of the program.

Q: Critical section.
Concurrent access to a shared resource can lead to unexpected results. So parts where the shared resoruces is accessed
is protected. This is called the critial section.

Q: Mutual exclusion (mutex).
One thread never enters the critical section while another concurrent thread enter its critical section

Q: Race condition.
The behaviour in which the output is dependent on the sequence or timing of other uncontrollable events

Q: Data race.
A data race occurs when two instructions from different threads access the same memory location and:

* at least one of these accesses is a write
* and there is no synchronization that is mandating any particular order among these accesses.

Locks and semaphores:

Q: What is the purose of mutex locks?
To make sure that only one thread enter its critial section to access shared resources at a time. 
This is to prevent race condition.

Q: If yoy had to make a choice between using a sempahore or a mutex to enforce mutex, what would you recommend and why?
Well. Mutex is good when we have a resource that only one thread should be able to access at a time since only
the process that acquires the lock can release it?

Q: How do you consruct a spinlock using the atomic test-and-set instruction?
Well like we did here. We create a lock variable and then use the TAS instruction to atomically set it to true
while also returning the previous value. This is done within a while-loop so if it is true when a thread tries
to acquire it that thread will be stuck in the while loop until the owner releases it (i.e. sets it to false)

This while loop should be placed before the critical section and the release after it. 


Performance analysis:

Discuss and analyze the results in test sumary table.
 * 
 * */