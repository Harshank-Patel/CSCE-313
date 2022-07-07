// Harshank Patel
// 527009145
// CSCE-313-512
// PA 0: Environment Setup, AddressSanitizer and GDB

#include <iostream>

//Blank A - Added Vector & standard template library
#include<vector>
using namespace std;


class node {
 //Blank B	- Made public as the access specifiers
 public:
 	int val;
 	node* next;
};


void create_LL(vector<node*>& mylist, int node_num){
    mylist.assign(node_num, NULL);

    //create a set of nodes
    for (int i = 0; i < node_num; i++) {
        
        //Blank C - Added the new node on the ith location
        mylist[i] = new node;


        mylist[i]->val = i;
        mylist[i]->next = NULL;
    }


    //create a linked list


    //Black Z - Added the -1 in the index so it doesnt go out of bounds! 
    for (int i = 0; i < node_num-1; i++) {
        mylist[i]->next = mylist[i+1];
    }
}

// *** NO CHHANGE *** //
int sum_LL(node* ptr) {
    int ret = 0;
    while(ptr) {
        cout <<ptr->val <<endl;
        ret += ptr->val;
        ptr = ptr->next;
    }
    return ret;
}


//Answer to Blank D - Delete Function
void del(vector<node *>&a, int size){
    for(int i = size-1; i >= 0; i--){
        delete a[i];
    }
    a.clear();
}


int main(int argc, char ** argv){
    const int NODE_NUM = 3;
    vector<node*> mylist;

    create_LL(mylist, NODE_NUM);
    int ret = sum_LL(mylist[0]); 
    cout << "The sum of nodes in LL is " << ret << endl;

    //Step4: delete nodes
    //Blank D - Added a delete function to clear all the memory leaks!
    del(mylist,NODE_NUM);


    return 0;
}