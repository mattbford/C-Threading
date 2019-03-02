#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <assert.h>

#define MAX_ITEMS 10
const int NUM_ITERATIONS = 200;
const int NUM_CONSUMERS  = 2;
const int NUM_PRODUCERS  = 2;

int producer_wait_count;     // # of times producer had to wait
int consumer_wait_count;     // # of times consumer had to wait
int histogram [MAX_ITEMS+1]; // histogram [i] == # of times list stored i items

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condc, condp;

int items = 0;

void* producer (void* v) {
  for (int i=0; i<NUM_ITERATIONS; i++) {
    // protects buffer from over-production
    pthread_mutex_lock(&mutex);
    if(items >= MAX_ITEMS) {
      while(items >= MAX_ITEMS) {
        pthread_cond_wait(&condp, &mutex);
      }
      producer_wait_count++;
    }

    //PRODUCE
    items++;
    histogram[items]++;
    //give greenlight to consume
    pthread_cond_signal(&condc);
    pthread_mutex_unlock(&mutex);

  }
  return NULL;
}

void* consumer (void* v) {
  for (int i=0; i<NUM_ITERATIONS; i++) {
    // protects buffer from over-consumption
    pthread_mutex_lock(&mutex);
    if(items <= 0 ) {
      while(items <= 0) {
        pthread_cond_wait(&condc, &mutex);
      }
      consumer_wait_count++;
    }
    //CONSUME
    items--;
    histogram[items]++;
    //give greenlight to produce
    pthread_cond_signal(&condp);
    pthread_mutex_unlock(&mutex);
  }
  return NULL;
}

int main (int argc, char** argv) {
  pthread_t t[4];

  // Create Threads and Join
  pthread_create(&t[0], NULL, producer, NULL);
  pthread_create(&t[1], NULL, producer, NULL);
  pthread_create(&t[2], NULL, consumer, NULL);
  pthread_create(&t[3], NULL, consumer, NULL);

  pthread_join(t[0], NULL);
  pthread_join(t[1], NULL);
  pthread_join(t[2], NULL);
  pthread_join(t[3], NULL);

  printf ("producer_wait_count=%d\nconsumer_wait_count=%d\n", producer_wait_count, consumer_wait_count);
  printf ("items value histogram:\n");
  int sum=0;
  for (int i = 0; i <= MAX_ITEMS; i++) {
    printf ("  items=%d, %d times\n", i, histogram [i]);
    sum += histogram [i];
  }
  printf("sum: %d\n", sum);
  assert (sum == sizeof (t) / sizeof (pthread_t) * NUM_ITERATIONS);
}
