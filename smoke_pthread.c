#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>

#define NUM_ITERATIONS 1000
//#define PTHREAD_COND_INITIALIZER {_PTHREAD_COND_SIG_init, {0}}

#ifdef VERBOSE
#define VERBOSE_PRINT(S, ...) printf (S, ##__VA_ARGS__);
#else
#define VERBOSE_PRINT(S, ...) ;
#endif

// PTHREAD_COND_INITIALIZER && PTHREAD_MUTEX_INITIALIZER giving me troubles wehn in the createAgent
// thus spawns this somewhat hacky solution

struct Agent {
  pthread_mutex_t mutex;
  pthread_cond_t  match;
  pthread_cond_t  paper;
  pthread_cond_t  tobacco;
  pthread_cond_t  smoke;
};

struct Agent* createAgent() {
  struct Agent* agent = malloc (sizeof (struct Agent));
  pthread_mutex_init(&agent->mutex, NULL);
  pthread_cond_init(&agent->match, NULL);
  pthread_cond_init(&agent->paper, NULL);
  pthread_cond_init(&agent->tobacco, NULL);
  pthread_cond_init(&agent->smoke, NULL);
  return agent;
}

//
// TODO
// You will probably need to add some procedures and struct etc.
//

//add a struct for smoker & smoking essentials, as well as their createfuncs

struct resources {
  struct Agent* agent;
  int paper;
  int match;
  int tobacco;
};

struct smoker {
  struct resources* resources;
  int type;
};

struct resources* createResources(struct Agent* agent) {
  struct resources* resources = malloc (sizeof (struct resources));
  resources->agent = agent;
  resources->paper = 0;
  resources-> match = 0;
  resources->tobacco = 0;
  return resources;
}

struct smoker* createSmoker(struct resources* resources, int type) {
  struct smoker* smoker = malloc (sizeof (struct smoker));
  smoker->resources = resources;
  smoker->type = type;
  return smoker;
}

/**
 * You might find these declarations helpful.
 *   Note that Resource enum had values 1, 2 and 4 so you can combine resources;
 *   e.g., having a MATCH and PAPER is the value MATCH | PAPER == 1 | 2 == 3
 */
enum Resource            {    MATCH = 1, PAPER = 2,   TOBACCO = 4};
char* resource_name [] = {"", "match",   "paper", "", "tobacco"};

//used to visualize interleaving
int debug = 1;

int signal_count [5];  // # of times resource signalled
int smoke_count  [5];  // # of times smoker with resource smoked

/**
 * This is the agent procedure.  It is complete and you shouldn't change it in
 * any material way.  You can re-write it if you like, but be sure that all it does
 * is choose 2 random reasources, signal their condition variables, and then wait
 * wait for a smoker to smoke.
 */
void* agent (void* av) {
  struct Agent* a = av;
  static const int choices[]         = {MATCH|PAPER, MATCH|TOBACCO, PAPER|TOBACCO};
  static const int matching_smoker[] = {TOBACCO,     PAPER,         MATCH};

  pthread_mutex_lock (&(a->mutex));
    for (int i = 0; i < NUM_ITERATIONS; i++) {
      int r = random() % 3;
      signal_count [matching_smoker [r]] ++;
      int c = choices [r];
      if (c & MATCH) {
        VERBOSE_PRINT ("match available\n");
        pthread_cond_signal (&(a->match));
      }
      if (c & PAPER) {
        VERBOSE_PRINT ("paper available\n");
        pthread_cond_signal (&(a->paper));
      }
      if (c & TOBACCO) {
        VERBOSE_PRINT ("tobacco available\n");
        pthread_cond_signal (&(a->tobacco));
      }
      VERBOSE_PRINT ("agent is waiting for smoker to smoke\n");
      pthread_cond_wait (&(a->smoke), &(a->mutex));
    }
  pthread_mutex_unlock (&(a->mutex));
  return NULL;
}

void* match (struct Agent* a, struct resources* resources, struct smoker* smoker) {
  //wait for match signal
  pthread_cond_wait(&(a->match), &(a->mutex));
  //smoker can smoke
  if (resources->tobacco > 0 && resources->paper > 0) {
    //smoker takes resources that are needed
    if(debug)
     printf("_");
    smoke_count[smoker->type]++;
    resources->tobacco--;
    resources->paper--;
    pthread_cond_signal(&(a->smoke));
  }
  //smoker cannot smoke
  else {
    resources->match++;
  }
}

void* paper (struct Agent* a, struct resources* resources, struct smoker* smoker) {
  //wait for paper signal
  pthread_cond_wait(&(a->paper), &(a->mutex));
  //smoker can smoke
  if (resources->tobacco > 0 && resources->match > 0) {
    //smoker takes resources that are needed
    if(debug)
     printf("-");
    smoke_count[smoker->type]++;
    resources->tobacco--;
    resources->match--;
    pthread_cond_signal(&(a->smoke));
  }
  //smoker cannot smoke
  else {
    resources->paper++;
  }
}

void* tobacco (struct Agent* a, struct resources* resources, struct smoker* smoker) {
  //wait for tobacco signal
  pthread_cond_wait(&(a->tobacco), &(a->mutex));
  //smoker can smoke
  if (resources->paper > 0 && resources->match > 0) {
    //smoker takes resources that are needed
    if(debug)
     printf("=");
    smoke_count[smoker->type]++;
    resources->paper--;
    resources->match--;
    pthread_cond_signal(&(a->smoke));
  }
  //smoker cannot smoke
  else {
    resources->tobacco++;
  }
}

void* smoker (void* av) {
 struct smoker* smoker = av;
 struct resources* resources = smoker->resources;
 struct Agent* a = resources->agent;

 pthread_mutex_lock(&(a->mutex));

 while(1) {
   if (smoker->type == MATCH) {
     match(a, resources, smoker);
   }
   else if (smoker->type == PAPER) {
     paper(a, resources, smoker);
   }
   else if (smoker->type == TOBACCO) {
     tobacco(a, resources, smoker);
   }

   //to prevent deadlocks we must send signals to other smokers
   if (resources->tobacco > 0 && resources->paper > 0) {
     pthread_cond_signal(&(a->match));
   }
   else if (resources->tobacco > 0 && resources->match > 0) {
     pthread_cond_signal(&(a->paper));
   }
   else if (resources->paper > 0 && resources-> match > 0) {
     pthread_cond_signal(&(a->tobacco));
   }
 }
 pthread_mutex_unlock(&(a->mutex));
}

int main (int argc, char** argv) {

  pthread_t t[4];

  struct Agent*  a = createAgent();
  struct resources* resources = createResources(a);

  struct smoker* matchSmoker = createSmoker(resources, MATCH);
  struct smoker* paperSmoker = createSmoker(resources, PAPER);
  struct smoker* tobaccoSmoker = createSmoker(resources, TOBACCO);

  pthread_create(&t[0], NULL, smoker, matchSmoker);
  pthread_create(&t[1], NULL, smoker, paperSmoker);
  pthread_create(&t[2], NULL, smoker, tobaccoSmoker);
  pthread_create(&t[3], NULL, agent, a);

  pthread_join (t[3], NULL);

  if(debug)
    printf("smoke match: %d  smoke paper: %d  smoke baccy: %d\n", smoke_count [MATCH], smoke_count [PAPER], smoke_count [TOBACCO]);
  assert (signal_count [MATCH]   == smoke_count [MATCH]);
  assert (signal_count [PAPER]   == smoke_count [PAPER]);
  assert (signal_count [TOBACCO] == smoke_count [TOBACCO]);
  assert (smoke_count [MATCH] + smoke_count [PAPER] + smoke_count [TOBACCO] == NUM_ITERATIONS);
  printf ("Smoke counts: %d matches, %d paper, %d tobacco\n",
          smoke_count [MATCH], smoke_count [PAPER], smoke_count [TOBACCO]);

}
