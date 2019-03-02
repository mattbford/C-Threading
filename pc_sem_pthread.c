#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#define MAX_ITEMS 10
const int NUM_ITERATIONS = 200;
const int NUM_CONSUMERS  = 2;
const int NUM_PRODUCERS  = 2;

int histogram [MAX_ITEMS+1]; // histogram [i] == # of times list stored i items
sem_t sem_empty;
sem_t sem_full;
sem_t avail;

int items = 0;

void* producer (void* v) {
  for (int i=0; i<NUM_ITERATIONS; i++) {

    sem_wait(&sem_empty);
    sem_wait(&avail);

    printf("-");
    items++;
    histogram[items]++;
    sem_post(&avail);
    if(items >= MAX_ITEMS)
      sem_post(&sem_full);
    else
      sem_post(&sem_empty);

  }
  return NULL;
}

void* consumer (void* v) {
  for (int i=0; i<NUM_ITERATIONS; i++) {

    sem_wait(&sem_full);
    sem_wait(&avail);

    printf("_");
    items--;
    histogram[items]++;
    sem_post(&avail);
    if(items <= 0)
      sem_post(&sem_empty);
    else
      sem_post(&sem_full);

  }
  return NULL;
}

int main (int argc, char** argv) {
  pthread_t t[4];

  sem_init(&sem_empty, 0, 1);
  sem_init(&sem_full, 0, 0);
  sem_init(&avail, 0, 1);

  // Create Threads and Join
  pthread_create(&t[0], NULL, producer, NULL);
  pthread_create(&t[1], NULL, producer, NULL);
  pthread_create(&t[2], NULL, consumer, NULL);
  pthread_create(&t[3], NULL, consumer, NULL);

  pthread_join(t[0], NULL);
  pthread_join(t[1], NULL);
  pthread_join(t[2], NULL);
  pthread_join(t[3], NULL);

  printf ("items value histogram:\n");
  int sum=0;
  for (int i = 0; i <= MAX_ITEMS; i++) {
    printf ("  items=%d, %d times\n", i, histogram [i]);
    sum += histogram [i];
  }
  printf("sum: %d\n", sum);
  assert (sum == sizeof (t) / sizeof (pthread_t) * NUM_ITERATIONS);

  sem_destroy(&sem_empty);
  sem_destroy(&sem_full);
  sem_destroy(&avail);

}
