Semaphore fullslots = 0;
Semaphore emptySlots = bufSize;;
Semaphore mutex = 1;

int pnumber = 0;
int cnumber = 0;

void Producer(item){
    while(true){
        emptySlots.P();
        mutex.P();
        Enqueue(item);
        pnumber++;
        mutex.V();
        if(pnumber == 5){
            pnumber = 0;
            fullslots.V();
            fullslots.V();
            fullslots.V();
            fullslots.V();
            fullslots.V();
        }
    }
}

void Consumer(){
    while(true){
        fullslots.P();
        mutex.P();
        item = Dequeue();
        cnumber++;
        mutex.V();
        if(cnumber == 5){
            cnumber = 0;
            emptySlots.V();
            emptySlots.V();
            emptySlots.V();
            emptySlots.V();
            emptySlots.V();
        }
        return item;
    }
}

// #include <iostream>
// #include <thread>
// #include <stdlib.h>
// #include <vector>
// #include <unistd.h>
// #include "Semaphore.h"
// using namespace std;

// Semaphore fullslots(0);
// Semaphore emptySlots(5);
// Semaphore mtx(1);

// vector<int> queue;
// int pnumber = 0;
// int cnumber = 0;

// void Producer(){
//     while(true){
//         emptySlots.P();
//         mutex.P();
//         queue.push_back(0);
//         cout <<"Added Zeros 0...\n";
//         pnumber++;
//         mutex.V();
        
//         if(pnumber == 5){
//             pnumber = 0;
//             fullslots.V();
//             fullslots.V();
//             fullslots.V();
//             fullslots.V();
//             fullslots.V();
//         }
//     }
// }


// void Consumer(){
//     while(true){
//         fullslots.P();
//         mutex.P();
//         queue.pop_back();
//         cout <<"Removed Zeros 0...\n";
//         cnumber++;
//         mutex.V();
//         if(cnumber == 5){
//             cnumber = 0;
//             emptySlots.V();
//             emptySlots.V();
//             emptySlots.V();
//             emptySlots.V();
//             emptySlots.V();
//         }
//     }
// }


// int main(){
//     vector <thread> producers;
//     vector<thread> consumers;
//     for(int i = 0; i < 5; i++){
//         producers.push_back(thread(Producer));
//         consumers.push_back(thread(Consumer));
//     }
//     for(int i = 0; i < 5; i++){
//         producers[i].join();
//         consumers[i].join();
//     }
// }