#include<iostream>
#include <iostream>
#include <thread>
#include <stdlib.h>
#include <vector>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include<thread>
using namespace std;

int count = 0;

void count_handler(int sig){
	count++;
	wait(0); //comment this and bottom for loop for correct answer!
}

int main(){
	signal(SIGCHLD, count_handler);
	for(int i = 0; i < 5; i++){
		int pid = fork();
		if(pid == 0){
			sleep(5);
			return 0;
		}
	}
	sleep(8);
	//Uncomment it for correct answer!
	// for(int i = 0; i < 5; i++){
	// 	wait(0);
	// }
	cout <<"Reaped : "<<count <<" children\n";
}