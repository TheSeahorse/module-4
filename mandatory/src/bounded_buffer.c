#include "bounded_buffer.h"

#include <string.h>  // strncmp()
#include <stdbool.h> // true, false
#include <assert.h>  // assert()
#include <ctype.h>   // isprint()
#include <stddef.h>  // NULL
#include <stdio.h>   // printf(), fprintf()
#include <stdlib.h>  // [s]rand()
#include <unistd.h>  // usleep(), sleep()
#include <pthread.h> // pthread_...
/*

An Arrow operator in C/C++ allows to access elements in Structures and Unions. 
It is used with a pointer variable pointing to a structure or union. 

Syntax:

(pointer_name)->(variable_name)
The -> operator in C or C++ gives the value held by variable_name to structure or union variable pointer_name.



*/

void buffer_init(buffer_t *buffer, int size)
{

  // Allocate the buffer array.
  tuple_t *array = malloc(size * sizeof(tuple_t));

  if (array == NULL)
  {
    perror("Could not allocate buffer array");
    exit(EXIT_FAILURE);
  }

  buffer->array = array; // We create an array using malloc size * (sizeOf * tuple)
  buffer->size = size;
  buffer->in = 0; // where to produce the next data
  buffer->out = 0; // where to consume the next data
  buffer->mutex = psem_init(1);
  buffer->data = psem_init(0); // Numbers of data in the buffer
  buffer->empty = psem_init(size); // To check if the buffer is emptys
}

void buffer_destroy(buffer_t *buffer)
{

  // Dealloacte the array.
  free(buffer->array);
  buffer->array = NULL;

  // Deallocate the mutex semaphore.
  psem_destroy(buffer->mutex);
  buffer->mutex = NULL;
  psem_destroy(buffer->data);
  buffer->data = NULL;
  psem_destroy(buffer->empty);
  buffer->empty = NULL;

  // TODO: Deallocate any other resources allocated to the bounded buffer.
}

void buffer_print(buffer_t *buffer)
{
  puts("");
  puts("---- Bounded Buffer ----");
  puts("");

  printf("size: %d\n", buffer->size);
  printf("  in: %d\n", buffer->in);
  printf(" out: %d\n", buffer->out);
  puts("");

  // Just a for loop that prints the a and b element of all the tuples: (a,b)
  for (int i = 0; i < buffer->size; i++)
  {
    printf("array[%d]: (%d, %d)\n", i, buffer->array[i].a, buffer->array[i].b);
  }

  puts("");
  puts("------------------------");
  puts("");
}

void buffer_put(buffer_t *buffer, int a, int b)
{
  /*
  Performs a wait() as the producer to check if the buffer is full 
  empty is init as the size of the buffer so each time something is added 
  it is first checked to be >= 0 and otherwise decremented by using wait()
  */
  psem_wait(buffer->empty); 

  /*
  Use a mutex to protect access to the critial section when something reads/writes to the buffer
  */
  psem_wait(buffer->mutex);

  // Insert the tuple (a, b) into the buffer.
  buffer->array[buffer->in].a = a;
  buffer->array[buffer->in].b = b;

  /* 
  add one (with wrap arpund in mind) to in which keeps track of where to produce the next data item using index
  Wrap around uses modulu to write to the first index after previously written to the last
  */
  buffer->in = (buffer->in + 1) % buffer->size;

  /*
  Releases the lock
  */
  psem_signal(buffer->mutex);

  /*
  Use signal to increment the semaphore data which keeps tracks of numbers of data in the buffer
  */
  psem_signal(buffer->data);
}

void buffer_get(buffer_t *buffer, tuple_t *tuple)
{
  /* 
  Use wait on data to see that it is >0 (otherwise it waits)
  and then decrement it since we have consumed one data from the buffer
  */
  psem_wait(buffer->data);

  /*
  Creates a lock to protect access to the critial section
  */
  psem_wait(buffer->mutex); 

  // Read the tuple (a, b) from the buffer.
  tuple->a = buffer->array[buffer->out].a;
  tuple->b = buffer->array[buffer->out].b;


  /*
  Updates out which keeps track of where to consume the next data item using index
  Use wrap around by using modulo with the size to start over on the first index after having consumed the last
  */
  buffer->out = (buffer->out + 1) % buffer->size;

  /*
  Releases the lock after leaving the critial section
  */
  psem_signal(buffer->mutex);

  /*
  Signals the empty to increment it since we just consumed one element and now have one more empty data
  */
  psem_signal(buffer->empty);
}



/*** 
Q: What do we mean by a counting semaphore?
A counting semaphore is a semaphore that can have a value over an unrestriced domain

Q: What happens when you do wait on counting semaphore?
If counter > 0, decrement counter,
otherwise WAIT until counter > 0, then decrement

Q: What happesn when you do signal on a countng semaphore?
Increments the semaphore counter

Q: Explain how producers and consumers are synchronized in order to:
  q: block consumers if the buffer is emtpy
  The consumer use wait on the semaphore data which keeps tracks of number of data items in the buffer,
  so if the buffer is empty data will be = 0, and when using wait it will WAIT under data > +

  q: block producers if the buffer is full.
  The producer use wait() on empty which is initlized to the size of the buffer and keep tracks of number of empty slots in the buffer
  So when calling wait on empty, the producer will WAIT if it's not >0 which means that it has 0 empty slots to write to
  
Q: Explain why mutex locks cannot be used to synchronize the blocking of consumers and producers.
A mutex lock should be taken and released by a single task, while a semaphore is a way to signal between tasks in which one
uses wait and one signal to achieve synchronization.

Q: Explain why you must ensure mutal exlusive (mutex) when updating the the buffer array and the in and out array indexes.
To control the access to shared resources

Q: Explain how you achive mutual exclusion (mutex) when updating the the buffer array and the in and out array indexes.
You can either use a mutex-lock which you take and release before and after the critial sections
Or use a binary semaphore which you wait and signal to before and after the critial section. 

*/