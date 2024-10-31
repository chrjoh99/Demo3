#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "common_threads.h"

#ifdef linux
#include <semaphore.h>  // Inlkuderer header-filen
#elif __APPLE__
#include "zemaphore.h"
#endif

#define MAX_MOD_READ 100
#define MAX_MOD_WRITE 100

/*
 * I den første delen av denne demoen kunne man kun kontrollere tilgangen til 
 * kritisk sektor, men ikke rekkefølgen på hvem som får tilgang. Ved å bruke et 
 * semaphore, får man en spesiell datatype hvor man kan bruke to funksjoner; 
 * "wait (down)" og "post (up)". Man kan også bruke semaphore for synkronisering, 
 * siden man kan bruke den som en condition variable.
 *
 * En tråd som teller ned fra en semaphore som er større enn null, vil bare 
 * gå videre. Men en tråd som teller ned fra en semaphore som er 
 * null (eller mindre), vil bli blokkert. Vi kan bruke dette som en lås for kritisk 
 * sektor, fordi alle som teller ned fra en verdi som er null vil bli blokkert.
 *
 * Hvis vi teller opp med en, altså gjør en "sem_post", så øker man verdien på s
 * emaphore'et med en. Hvis det er en eller flere tråder som venter på 
 * semaphoret, vekkes en av dem.
 *
 * Ved linux er i praksis semaphore aldri mindre enn null, men vi kan tenkte 
 * på det som mindre enn null. Hvis en tråd teller ned på et semaphore som er 
 * null, så blir det -1. Teller en tråd til ned på semaphore, så blir det -2. 
 * Da kan vi tenkte på at tallet -2 representerer antallet tråder som sover 
 * på dette semaphore.
 */

typedef struct _rwlock_t {
  sem_t writelock;
  sem_t lock;
  sem_t turnstile;      // Deklarerer et nytt semaphore, kalt "turnstile"
  int readers;
} rwlock_t;

void rwlock_init(rwlock_t *lock) {
  lock->readers = 0;
  Sem_init(&lock->lock, 1);
  Sem_init(&lock->writelock, 1);
  Sem_init(&lock->turnstile, 1); // Initialisere turnstile til 1
}

void rwlock_acquire_readlock(rwlock_t *lock) { // Skal lese
  Sem_wait(&lock->turnstile);    // Venter på turnstile 
  Sem_post(&lock->turnstile);    // Releaser turnstile

  Sem_wait(&lock->lock);
  lock->readers++;
  if (lock->readers == 1) Sem_wait(&lock->writelock);
  Sem_post(&lock->lock);
}

void rwlock_release_readlock(rwlock_t *lock) { // Ferdig med å lese
  Sem_wait(&lock->lock);
  lock->readers--;
  if (lock->readers == 0) Sem_post(&lock->writelock);
  Sem_post(&lock->lock);
}

void rwlock_acquire_writelock(rwlock_t *lock) { // Skal skrive
  Sem_wait(&lock->turnstile);    // Venter på turnstile
  Sem_wait(&lock->writelock); 
}

void rwlock_release_writelock(rwlock_t *lock) { // Fredig med å skrive
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


/*

Utskriften før bruk av turnstile vil se slik ut:

Kommando: ./reader-writer 10 2 10

begin
reader 0 reads 0
reader 3 reads 0
reader 4 reads 0
reader 0 reads 0
reader 3 reads 0
reader 6 reads 0
reader 0 reads 0
reader 3 reads 0
reader 2 reads 0
reader 1 reads 0
reader 7 reads 0
reader 9 reads 0
reader 4 reads 0
reader 8 reads 0
reader 3 reads 0
reader 0 reads 0
reader 2 reads 0
reader 1 reads 0
reader 7 reads 0
reader 9 reads 0
reader 8 reads 0
reader 3 reads 0
reader 0 reads 0
reader 2 reads 0
reader 7 reads 0
reader 1 reads 0
reader 9 reads 0
reader 2 reads 0
reader 8 reads 0
reader 7 reads 0
reader 3 reads 0
reader 0 reads 0
reader 5 reads 0
reader 2 reads 0
reader 1 reads 0
reader 8 reads 0
reader 6 reads 0
reader 4 reads 0
reader 8 reads 0
reader 7 reads 0
reader 9 reads 0
reader 0 reads 0
reader 3 reads 0
reader 2 reads 0
reader 1 reads 0
reader 6 reads 0
reader 7 reads 0
reader 8 reads 0
reader 1 reads 0
reader 0 reads 0
reader 9 reads 0
reader 3 reads 0
reader 2 reads 0
reader 8 reads 0
reader 7 reads 0
reader 6 reads 0
reader 1 reads 0
reader 9 reads 0
reader 2 reads 0
reader 0 reads 0
reader 3 reads 0
reader 1 reads 0
reader 8 reads 0
reader 7 reads 0
reader 6 reads 0
reader 2 reads 0
reader 8 reads 0
reader 7 reads 0
reader 1 reads 0
reader 9 reads 0
reader 3 reads 0
reader 2 reads 0
reader 7 reads 0
reader 8 reads 0
reader 0 reads 0
reader 1 reads 0
reader 6 reads 0
reader 9 reads 0
reader 4 reads 0
reader 6 reads 0
reader 9 reads 0
reader 5 reads 0
reader 6 reads 0
reader 9 reads 0
reader 4 reads 0
reader 6 reads 0
writer 0 writes 1
writer 1 writes 2
writer 0 writes 3
reader 4 reads 3
reader 5 reads 3
reader 6 reads 3
reader 4 reads 3
writer 1 writes 4
writer 0 writes 5
reader 4 reads 5
reader 5 reads 5
reader 4 reads 5
reader 5 reads 5
reader 4 reads 5
reader 5 reads 5
writer 0 writes 6
writer 1 writes 7
reader 5 reads 7
writer 0 writes 8
reader 5 reads 8
writer 1 writes 9
reader 5 reads 9
writer 0 writes 10
writer 1 writes 11
reader 5 reads 11
writer 0 writes 12
writer 1 writes 13
writer 0 writes 14
writer 1 writes 15
writer 0 writes 16
writer 1 writes 17
writer 0 writes 18
writer 1 writes 19
writer 1 writes 20
end: value 20


Utskriften etter bruk av turnstile vil se slik ut:

Kommando: ./reader-writer 10 2 10

begin
reader 0 reads 0
reader 1 reads 0
reader 2 reads 0
reader 0 reads 0
reader 3 reads 0
reader 1 reads 0
reader 2 reads 0
reader 0 reads 0
reader 1 reads 0
reader 2 reads 0
reader 3 reads 0
reader 0 reads 0
reader 4 reads 0
reader 1 reads 0
reader 2 reads 0
reader 4 reads 0
reader 2 reads 0
reader 3 reads 0
reader 1 reads 0
reader 0 reads 0
reader 5 reads 0
reader 2 reads 0
reader 3 reads 0
reader 0 reads 0
reader 4 reads 0
reader 1 reads 0
reader 2 reads 0
reader 4 reads 0
reader 0 reads 0
reader 1 reads 0
reader 3 reads 0
reader 7 reads 0
reader 2 reads 0
reader 1 reads 0
reader 0 reads 0
reader 4 reads 0
reader 3 reads 0
reader 5 reads 0
reader 2 reads 0
reader 0 reads 0
reader 1 reads 0
reader 3 reads 0
reader 4 reads 0
reader 7 reads 0
reader 2 reads 0
reader 0 reads 0
reader 1 reads 0
reader 5 reads 0
reader 4 reads 0
reader 3 reads 0
writer 1 writes 1
writer 0 writes 2
reader 4 reads 2
reader 9 reads 2
writer 0 writes 3
reader 3 reads 3
reader 5 reads 3
reader 7 reads 3
reader 9 reads 3
reader 8 reads 3
reader 4 reads 3
writer 1 writes 4
writer 0 writes 5
reader 3 reads 5
reader 5 reads 5
reader 7 reads 5
writer 0 writes 6
reader 9 reads 6
reader 4 reads 6
reader 8 reads 6
writer 1 writes 7
reader 9 reads 7
reader 5 reads 7
reader 7 reads 7
writer 1 writes 8
writer 0 writes 9
reader 5 reads 9
reader 7 reads 9
reader 9 reads 9
reader 8 reads 9
writer 1 writes 10
writer 0 writes 11
reader 5 reads 11
reader 7 reads 11
writer 0 writes 12
reader 9 reads 12
reader 5 reads 12
writer 1 writes 13
reader 9 reads 13
reader 7 reads 13
writer 0 writes 14
reader 5 reads 14
reader 7 reads 14
reader 9 reads 14
writer 1 writes 15
writer 0 writes 16
reader 8 reads 16
writer 0 writes 17
writer 1 writes 18
reader 9 reads 18
reader 7 reads 18
reader 8 reads 18
reader 6 reads 18
writer 1 writes 19
reader 9 reads 19
reader 8 reads 19
writer 1 writes 20
reader 8 reads 20
reader 6 reads 20
reader 8 reads 20
reader 6 reads 20
reader 8 reads 20
reader 6 reads 20
reader 8 reads 20
reader 6 reads 20
reader 6 reads 20
reader 6 reads 20
reader 6 reads 20
reader 6 reads 20
reader 6 reads 20
end: value 20

*/

