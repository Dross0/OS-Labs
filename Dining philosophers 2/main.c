#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define PHILO 5
#define DELAY 30000
#define FOOD 50

pthread_mutex_t forks[PHILO];
pthread_mutex_t foodlock;
pthread_t phils[PHILO];
pthread_cond_t cond;
int sleep_seconds = 0;

void *philosopher (void *id);
int food_on_table ();
void get_forks(int, int, int);
void down_forks (int, int);

int main (int argc, char **argv)
{
  int i;      
      if (argc == 2)
    sleep_seconds = atoi (argv[1]);

  pthread_mutex_init (&foodlock, NULL);
  pthread_cond_init(&cond,NULL);
  for (i = 0; i < PHILO; i++)
    pthread_mutex_init (&forks[i], NULL);
  for (i = 0; i < PHILO; i++)
    pthread_create (&phils[i], NULL, philosopher,(void *)&i);
  for (i = 0; i < PHILO; i++)
    pthread_join (phils[i], NULL);
  return 0;
}

void * philosopher (void *num)
{
  int f;

  int id = *((int*)num);
  printf ("Philosopher %d sitting down to dinner.\n", id);
  int right_fork = id;
  int left_fork = id + 1;
 
  /* Wrap around the forks. */
  if (left_fork == PHILO)
    left_fork = 0;
 
  while (f = food_on_table ()) {
    
    /* Thanks to philosophers #1 who would like to 
     * take a nap before picking up the forks, the other
     * philosophers may be able to eat their dishes and 
     * not deadlock.
     */
    if (id == 1)
      sleep (sleep_seconds);

    printf ("Philosopher %d: get dish %d.\n", id, f);
    get_forks(id, left_fork, right_fork);

    printf ("Philosopher %d: eating.\n", id);
    usleep (DELAY * (FOOD - f + 1));
    down_forks (left_fork, right_fork);
  }
  printf ("Philosopher %d is done eating.\n", id);
  return (NULL);
}

int food_on_table ()
{
  static int food = FOOD;

  pthread_mutex_lock (&foodlock);
  if (food > 0) {
    food--;
  }
  int myfood = food;
  pthread_mutex_unlock (&foodlock);
  return myfood;
}

void get_forks(int phil, int forkLeft, int forkRight) {
    int getForks = 0;
    while (getForks == 0) {
        pthread_mutex_lock(&forks[forkLeft]);
        printf("Philosopher#%d: got %s fork %d\n", phil, "left", forkLeft);
      
       while(getForks == 0){
       	  if (pthread_mutex_trylock(&forks[forkRight]) == 0) {
             printf("Philosopher#%d: got %s fork %d\n", phil, "right", forkRight);
             getForks = 1;
          } else {
             printf("Philosopher#%d: couldn't get %s fork %d\n", phil, "right", forkRight);
             pthread_cond_wait(&cond, &forks[forkLeft]);
          }

       }	
        
    }
}

void down_forks (int f1, int f2)
{
  pthread_mutex_unlock (&forks[f1]);
  pthread_mutex_unlock (&forks[f2]);
  pthread_cond_broadcast(&cond);
}
