#include <iostream>
#include <thread>
#include <stdlib.h>
#include <vector>
#include <unistd.h>
#include "Semaphore.h"
using namespace std;

#define NA 1
#define NB 2
#define NC 1

int buffer=0;
Semaphore A(1);
Semaphore B(0);
Semaphore C(0);

Semaphore mtx(1);

int nAdone=0;
int nBdone=0;
int nCdone=0;

void a_func(int pno){
	while(true){
		
		A.P(); //waits and decrements by 1
		cout <<endl;
		

		mtx.P(); //waits and decrements by 1
		buffer++;
		// cout <<"In A: A Mutex : "<<A.value << " B Mutex : "<<B.value <<" C Mutex : "<<C.value<<endl;
		cout <<"Thread A ["<<pno<<"] is on buffer : "<<buffer<<endl;
		usleep (900000);
		mtx.V(); //Increments by 1 and unlock()

		mtx.P(); //waits and decrements by 1
		nAdone++;
		if(nAdone == NA){
			for(int i = 0; i < NB; i++){
				B.V(); //Increments by 1 and unlock()
			}
			nAdone = 0; // reset the counter
		}
		mtx.V(); //Increments by 1 and unlock()

	}
}

void b_func(int pno){
	while(true){
		B.P(); //waits and decrements by 1
		// cout <<"In B: A Mutex : "<<A.value << " B Mutex : "<<B.value <<" C Mutex : "<<C.value<<endl;

		mtx.P(); //waits and decrements by 1
		buffer++;
		cout <<"Thread B ["<<pno<<"] is on buffer : "<<buffer<<endl;
		usleep (900000);
		mtx.V(); //Increments by 1 and unlock()

		mtx.P(); //waits and decrements by 1
		nBdone++;
		if(nBdone == NB){
			for(int i = 0; i < NC; i++){
				C.V(); //Increments by 1 and unlock()
			}
			nBdone = 0; // reset the counter
		}
		mtx.V(); //Increments by 1 and unlock()

	}
}

void c_func(int pno){
	while(true){
		C.P(); //waits and decrements by 1
		// cout <<"In C: A Mutex : "<<A.value << " B Mutex : "<<B.value <<" C Mutex : "<<C.value<<endl;

		mtx.P(); //waits and decrements by 1
		buffer++;
		cout <<"Thread C ["<<pno<<"] is on buffer : "<<buffer<<endl;
		usleep (900000);
		mtx.V(); //Increments by 1 and unlock()

		mtx.P(); //waits and decrements by 1
		nCdone++;
		if(nCdone == NC){
			for(int i = 0; i < NA; i++){
				A.V(); //Increments by 1 and unlock()
			}
			nCdone = 0; // reset the counter
		}
		mtx.V(); //Increments by 1 and unlock()
	}
}

int main(){
	vector<thread> a;
	vector<thread> b;
	vector<thread> c;
	//
	for(int i=0; i< 100; i++){
		a.push_back(thread(a_func, i+1));
		b.push_back(thread(b_func, i+1));
		c.push_back(thread(c_func, i+1));
	}

	//Join All The Threads
	for(int i=0; i<100; i++){
		a[i].join();
		b[i].join();
		c[i].join();
	}
}


/*
// each producer gets an id, which is pno	
void producer_function(int pno){
	int count = 0; // each producer threads has its own count
	while(true) {
		// do necessary wait
		consumerdone.P(); //waits and decrements by 1		// wait for the buffer to be empty
		// after wait is done, do the produce operation
		// you should not need to change this block
		mtx.P(); //waits and decrements by 1
		buffer ++;
		cout << "Producer [" << pno << "] left buffer=" << buffer << endl;
		mtx.V(); //Increments by 1 and unlock()
		// now do whatever that would indicate that the producers are done
		// in this case, the single producer is waking up all NC consumers
		// this will have to change when you have NP producers
		mtx.P(); //waits and decrements by 1
		npdone ++;
		if(npdone == NP){ // it is the last one 
			for(int i = 0; i < NC; i++){
				producerdone.V(); //Increments by 1 and unlock() // so wake up the producer
			}
			npdone = 0; // reset the counter
		}
		mtx.V(); //Increments by 1 and unlock()
	}
}
// each consumer gets an id cno
void consumer_function(int cno){
	while(true){
		//do necessary wait
		producerdone.P(); //waits and decrements by 1
		// each consumer reads the buffer		
		// you should not need to change this block
		mtx.P(); //waits and decrements by 1
		cout << ">>>>>>>>>>>>>>>>>>>>Consumer [" <<cno<<"] got <<<<<<<<<<" << buffer << endl;
		mtx.V(); //Increments by 1 and unlock()
		usleep(5000000);

		// now do whatever necessary that would indicate that the consumers are all done
		mtx.P(); //waits and decrements by 1
		ncdone ++;
		if(ncdone == NC){ // it is the last one 
			for(int i = 0; i < NP; i++){
				consumerdone.V(); //Increments by 1 and unlock() // so wake up the producer
			}
			ncdone = 0; // reset the counter
		}
		mtx.V(); //Increments by 1 and unlock()
	}
}

int main(){
	vector<thread> producers;
	vector<thread> consumers;

	for(int i=0; i< 100; i++)
		producers.push_back(thread(producer_function, i+1));

	for(int i=0; i< 300; i++)
		consumers.push_back(thread(consumer_function, i+ 1));

	
	for(int i=0; i<consumers.size(); i++)
		consumers [i].join();
	
	for(int i=0; i<producers.size(); i++)
		producers [i].join();
	
}


*/