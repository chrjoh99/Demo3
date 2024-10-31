#include <stdio.h> 
#include <pthread.h> // Datatypen pthread_t er bare int'er
#include <string.h> 

/*
 * Når man lager flere tråder, og begynner å dele data mellom trådene, blir programmet 
 * mer uforutsigbart. Der hvor flere tråder behandler de samme dataene, kaller man det 
 * for kritisk sektor. I en kritisk sektor kan som regel bare en tråd være om gangen. 
 * Da må man låse av slik at bare en tråd er der om gangen.
 * 
 * Derfor bruker man funksjonene "mutex_lock" og "mutex_unlock" fra 
 * pthread-biblioteket.
 * 
 * Mutex er en lås, og selve ordet "mutex" står for mutual exclusion, altså       
 * er prinsipp der hvor man utelukker alle andre tråder enn seg selv.
 */

typedef struct {    
	char name[100]; 
	int age; 
} Person; 

Person person; 

pthread_mutex_t mutex; // Deklarer mutex her

void *thread1_func(void *arg) { 
	int counter=0; 
	int noerror=1; 
	
	while(1) { 
		counter++; 
		pthread_mutex_lock(&mutex);    // Låser før endringer (kritisk sektor)
		
		strcpy(person.name, "Alice");
		person.age = 30;             
		
		if (strcmp(person.name, "Alice") == 0 && person.age != 30) {
			printf("Thread 1: Alice is not 30! she is %d\n", person.age);
			noerror=0; 
		} 
		
		pthread_mutex_unlock(&mutex);   // Låser opp etter kritisk sektor 
		
		if (noerror == 1 && (counter % 1000000 == 0)) { 
			printf("Hey you rock! Alice's OK!\n"); 
		} 
	} 
	
	return NULL; 
} 

void *thread2_func(void *arg) { 
	int counter=0; 
	int noerror=1; 
	
	while(1) { 
		counter++; 
		pthread_mutex_lock(&mutex);     // Låser før kritisk sektor
		
		strcpy(person.name, "Bob"); 
		person.age = 25;            
		
		if (strcmp(person.name, "Bob") == 0 && person.age != 25) {
			printf("Thread 2: Bob is not 25! he is %d\n", person.age);
			noerror=0; 
		} 
		
		pthread_mutex_unlock(&mutex);   // Låser opp etter kritisk sektor
		
		if (noerror == 1 && (counter % 1000000 == 0)) { 
			printf("Hey you rock! Bob's OK!\n"); 
		} 
	} 
	
	return NULL; 
} 

int main() { 
	pthread_t thread1, thread2; 
	
	pthread_mutex_init(&mutex, NULL);      // Initialiserer mutex med adressen til mutex 
	
	pthread_create(&thread1, NULL, thread1_func, NULL); 
	pthread_create(&thread2, NULL, thread2_func, NULL); 
	
	pthread_join(thread1, NULL); 
	pthread_join(thread2, NULL);
	
	return 0; 
}


/*

Utskriften før mutex/locking vil se slik ut:

Thread 2: Bob is not 25! he is 30
Thread 2: Bob is not 25! he is 30
Thread 1: Alice is not 30! she is 25
Thread 1: Alice is not 30! she is 25
Thread 1: Alice is not 30! she is 25
Thread 2: Bob is not 25! he is 30
Thread 1: Alice is not 30! she is 25


Utskriften med mutex/locking vil se slik ut:

Hey you rock! Bob's OK!
Hey you rock! Alice's OK!
Hey you rock! Bob's OK!
Hey you rock! Alice's OK!
Hey you rock! Bob's OK!
Hey you rock! Alice's OK!
Hey you rock! Bob's OK!
Hey you rock! Alice's OK!
Hey you rock! Bob's OK!
Hey you rock! Alice's OK!
Hey you rock! Bob's OK!
Hey you rock! Bob's OK!
Hey you rock! Alice's OK!
Hey you rock! Bob's OK!
Hey you rock! Alice's OK!
Hey you rock! Bob's OK!
Hey you rock! Alice's OK!
Hey you rock! Bob's OK!
Hey you rock! Alice's OK!
Hey you rock! Bob's OK!
Hey you rock! Alice's OK!
Hey you rock! Bob's OK!
Hey you rock! Bob's OK!
Hey you rock! Alice's OK!
Hey you rock! Bob's OK!

*/
