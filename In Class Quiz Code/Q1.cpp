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

#define M 32
// #define M 22
int monkey_count[2] = {0,0};
//Vector of Semaphores/Mutex!
vector<Semaphore*> mutexes;
//Capacity of the Bridge!
Semaphore capacity(M);

Semaphore dirmtx(1);

//Extra Mutex I added!
Semaphore incdecmtx(1);

void WaitUntilSafe(int dir,int weight){
	//Lock the direction which will prevent the other end of the monkeys to cross
	mutexes[dir]->P();
	//Increase the counter that startedd to cross.
	monkey_count[dir]++;
	//Check for the first monkey1
	if(monkey_count[dir] == 1){
		//Lock the direction which will prevent the other end of the monkeys to cross
		dirmtx.P();
	}
	//Tell the mutexes of dir to open up!
	mutexes[dir]->V();
	incdecmtx.P(); // make sure that the increment and decreament is not in a race cond
	for(int i = 0; i < weight; i++){
		//Put it inside a for loop so each of the monkey's weight is added from the capacity.
		capacity.P();
	}
	//Unlock the increment mutex!
	incdecmtx.V();
}

void DoneWithCrossing(int dir,int weight){
	//The Monkeys are locked from coming in since the
	mutexes[dir]->P();
	//Keep track of number of monkeys that crossed!
	monkey_count[dir]--;
	if(monkey_count[dir] == 0){
		//wait for it to be done!
		dirmtx.V();
	}
	//No need for mutex since the capacity.V() is not holding up any of the new monkeys!
	mutexes[dir]->V();
	for(int i = 0; i < weight; i++){
		//Put it inside a for loop so each of the monkey's weight is removed from the capacity.
		capacity.V();
	}
}

void CrossRavine(int monkeyid, int dir, int weight){
    cout << "Monkey [id=" << monkeyid << ", wt=" << weight <<", dir=" << dir <<"] is ON the rope" << endl;
    sleep (rand()%5);
	// sleep(2);
    cout << "Monkey [id=" << monkeyid << ", wt=" << weight <<", dir=" << dir <<"] is OFF the rope" << endl;
}

void monkey(int monkeyid, int dir, int weight) {
	WaitUntilSafe(dir, weight); // work finished!
	CrossRavine(monkeyid, dir, weight); // given no work needed!
	DoneWithCrossing(dir, weight); // work finished!
}

int main(){
	//Number of Monkey's
	int C = 500;
	srand(time(0));
	vector<thread> monkeys;
	// int zeroVal = 0;
	// int oneVal = 0;
	for(int i = 0; i < 2; i++) {
		mutexes.push_back(new Semaphore(1));
	}
	cout <<C <<" monkeys trying to cross the bridge of weight "<<M <<endl <<endl <<endl;
	for(int i=0; i<C; i++){
		int dir = rand() % 2; // random 0 or 1 for the direction!
		int weight = (rand() % 20) + 1; // wieght for the monkeys randomly generated!
		monkeys.push_back(thread(monkey, i+1, dir, weight));
	}
	// cout <<"Zero Monkeys :"<<zeroVal<<endl;
	// cout <<"One Monkeys :"<<oneVal <<endl;
	for(int i=0; i<C; i++){
		monkeys[i].join();
	}	
}

