//Harshank Patel
//527009145
#include <ctime>
#include <thread>
#include <vector>
#include <cstdlib>
#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include "Semaphore.h"
using namespace std;

enum flagType { THINK, HUNGRY, EAT};

//DOWN = Lock = .P
//UP = Unlock = .V

class FLAGS{
	public:
		int val;
		FLAGS(){
			val = flagType::THINK;
		}
};

Semaphore me(1);
vector<Semaphore*> s;
vector<FLAGS*>pflag;

void test(int i) {            /* Let phil[i] eat, if waiting */
	if ( pflag[i]->val == flagType::HUNGRY 
		&& pflag[(i + 4) % 5]->val != flagType::EAT 
			&& pflag[(i + 1) % 5]->val != flagType::EAT) {
		// pflag[i]->val = flagType::EAT;
		s[i]->V();  //UP(s[i])
	}
}

void drop_chopsticks(int i) {
	me.P();  //DOWN(me);      /* critical section */
	test((i + 4) % 5);        /* Let phil. on left eat if possible */
	test((i + 1) % 5);        /* Let phil. on rght eat if possible */
	me.V();  //UP(me);        /* up critical section */
}

void take_chopsticks(int i) {
	me.P();  //DOWN(me);       /* critical section */
	pflag[i]->val = flagType::HUNGRY;
	test(i);
	me.V();  //UP(me);         /* end critical section */
	s[i]->P();  //DOWN(s[i])   /* Eat if enabled */
}

void philosopher(int i){
	while(true){
		pflag[i]->val = flagType::THINK;
		sleep((rand() % 5) + 1); // THINKING
		take_chopsticks(i);
		cout <<"Starting to eat: "<<i<<endl;
		pflag[i]->val = flagType::EAT;
       	sleep((rand() % 5) + 1); //EATING
		pflag[i]->val = flagType::THINK;
		cout <<"Eating done for philosopher: "<<i<<endl;
		drop_chopsticks(i);
	}
}

int main(){
	srand(time(0));
	for(int i = 0; i < 5; i++){
		s.push_back(new Semaphore(0));
		pflag.push_back(new FLAGS());
	}
	vector<thread> philosophers;
	//Create threads!
	for(int i=0; i< 5; i++){
		philosophers.push_back(thread(philosopher, i));
	}
	//Join All The Threads
	for(int i=0; i<5; i++){
		philosophers[i].join();	
	}
}