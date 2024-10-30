#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "common_threads.h"

#ifdef linux
#include <semaphore.h>
#elif __APPLE__
#include "zemaphore.h"
#endif

#define MAX_MOD_READ 100
#define MAX_MOD_WRITE 100

typedef struct _rwlock_t {
  sem_t writelock;
  sem_t lock;
  sem_t turnstile;               // Ny semaphore for å håndtere tilgang
  int readers;
} rwlock_t;

void rwlock_init(rwlock_t *lock) {
  lock->readers = 0;
  Sem_init(&lock->lock, 1);
  Sem_init(&lock->writelock, 1);
  Sem_init(&lock->turnstile, 1); // Initialisere turnstile
}

void rwlock_acquire_readlock(rwlock_t *lock) {
  Sem_wait(&lock->turnstile);    // Venter på turnstile 
  Sem_post(&lock->turnstile);    // Releaser turnstile

  Sem_wait(&lock->lock);
  lock->readers++;
  if (lock->readers == 1) Sem_wait(&lock->writelock);
  Sem_post(&lock->lock);
}

void rwlock_release_readlock(rwlock_t *lock) {
  Sem_wait(&lock->lock);
  lock->readers--;
  if (lock->readers == 0) Sem_post(&lock->writelock);
  Sem_post(&lock->lock);
}

void rwlock_acquire_writelock(rwlock_t *lock) { 
  Sem_wait(&lock->turnstile);    // Venter på turnstile
  Sem_wait(&lock->writelock); 
}

void rwlock_release_writelock(rwlock_t *lock) { 
  Sem_post(&lock->writelock); 
  Sem_post(&lock->turnstile);    // Releaser turnstile
}

int loops;
int value = 0;

rwlock_t lock;

void *reader(void *arg) {
  int i;
  for (i = 0; i < loops; i++) {
    rwlock_acquire_readlock(&lock);
    printf("reader %ld reads %d\n", (intptr_t)arg, value);
    usleep(random() % MAX_MOD_READ);
    rwlock_release_readlock(&lock);
    usleep(random() % MAX_MOD_READ);
  }
  return NULL;
}

void *writer(void *arg) {
  int i;
  for (i = 0; i < loops; i++) {
    rwlock_acquire_writelock(&lock);
    value++;
    printf("writer %ld writes %d\n", (intptr_t)arg, value);
    usleep(random() % MAX_MOD_WRITE);
    rwlock_release_writelock(&lock);
    usleep(random() % MAX_MOD_WRITE);
  }
  return NULL;
}

int main(int argc, char *argv[]) {
  assert(argc == 4);
  int num_readers = atoi(argv[1]);
  int num_writers = atoi(argv[2]);
  loops = atoi(argv[3]);

  pthread_t pr[num_readers], pw[num_writers];

  rwlock_init(&lock);

  printf("begin\n");

  intptr_t i;
  for (i = 0; i < num_readers; i++)
    Pthread_create(&pr[i], NULL, reader, (void *)i);
  for (i = 0; i < num_writers; i++)
    Pthread_create(&pw[i], NULL, writer, (void *)i);

  for (i = 0; i < num_readers; i++) Pthread_join(pr[i], NULL);
  for (i = 0; i < num_writers; i++) Pthread_join(pw[i], NULL);

  printf("end: value %d\n", value);

  return 0;
}
