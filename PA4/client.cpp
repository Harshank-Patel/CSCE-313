#include <queue>
#include <mutex>
#include <chrono>
#include <string>
#include <thread>
#include <vector>
#include <time.h>
#include <stdio.h>
#include <signal.h>
#include <assert.h>
#include <unistd.h>
#include <stdlib.h>
#include <iostream>
#include <stdint.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <condition_variable>

using namespace std;

#include "common.h"
#include "Histogram.h"
#include "BoundedBuffer.h"
#include "FIFOreqchannel.h"
#include "HistogramCollection.h"

time_t start_time;
__int64_t amountRecieved;
__int64_t totalAmount;
//For File
bool outFile = false;
HistogramCollection hc;

void sigFunc(int sig) {
    // printf("Alarm!  Alarm!  got signal %d at time %d\n", sig, time(NULL)-start_time);
    if(!outFile){
        system("clear");
	    hc.print ();
    }
    else{
        system("clear");
        cout <<"\nFile Download in Process...\n    "<<amountRecieved*100 / totalAmount <<"% done...   "<<endl;
    }
    alarm(2);
}

struct Response{
    int person;
    double ecg;
};

void patient_thread_function(int p, int n, BoundedBuffer* requestBuffer){
    /* What will the patient threads do? */
    //will get the message!
    datamsg d(p,0.00,1);
    for(int i = 0; i < n; i++) {
        requestBuffer -> push ((char *) &d, sizeof(datamsg));
        d.seconds += 0.004;
    }
}

void worker_thread_function(BoundedBuffer * requestBuffer, FIFORequestChannel *wchan, BoundedBuffer *responseBuffer, int mb){
   char buf [1024];
   char recvbuf[mb];
   while(true) {
       requestBuffer->pop(buf, sizeof(buf));
       MESSAGE_TYPE* m = (MESSAGE_TYPE *) buf;

       if(*m == QUIT_MSG){
           //Proper Cleaning After Gettingg Kicked Out of the loop
           wchan -> cwrite(m, sizeof(MESSAGE_TYPE));
           delete wchan;
           break;
       }

       if(*m == DATA_MSG){
           datamsg * d = (datamsg *) buf;
           wchan->cwrite(d,sizeof(datamsg));
           double ecg;
           wchan -> cread(&ecg, sizeof(double));
           Response r{d->person, ecg};
           responseBuffer->push((char *) &r, sizeof(r));
       }

       if(*m == FILE_MSG){
           //.... File Will Be Handeled Here ....//
           filemsg * fm = (filemsg*) buf;
           string fname = (char *)(fm + 1);

           int sz = sizeof(filemsg)+fname.size()+1;
           wchan->cwrite(buf,sz);
           wchan->cread(recvbuf,mb);


           //write it now!
           string recvfname = "recieved/"+fname;
           FILE*fp = fopen(recvfname.c_str(),"r+");

           fseek(fp,fm->offset, SEEK_SET);
           fwrite(recvbuf,1,fm->length,fp);
           fclose(fp);
           //
       }
   }
}

void histogram_thread_function (BoundedBuffer * responseBuffer, HistogramCollection *hc){
   char buf[1024];
   while(true){
       responseBuffer->pop(buf,1024);

       Response *r = (Response *) buf;
       if(r->person == -1){
           break;
        }
        else{
           hc->update(r->person, r->ecg);
       }
    }
}

void file_thread_function(string fileName, BoundedBuffer* requestBuffer, FIFORequestChannel *chan, int mb){
    //1. Create the file!    
    char buf[1024];
    filemsg f(0,0);
    
    memcpy(buf, &f, sizeof(f));
    strcpy(buf+sizeof(f), fileName.c_str());
    chan->cwrite(buf,sizeof(f)+fileName.size() + 1);
    __int64_t filelength;
    chan->cread(&filelength, sizeof(filelength));

    string recvfname = "recieved/"+fileName;
    FILE*fp = fopen(recvfname.c_str(),"w");
    fseek(fp, filelength, SEEK_SET);
    fclose(fp);

    //2. Generate All the file messages
    filemsg *fm = (filemsg *) buf;
    __int64_t remlength = filelength;
    totalAmount = filelength;
    while(remlength > 0){
        fm->length = min(remlength, (__int64_t) mb);
        requestBuffer->push(buf,sizeof(filemsg) + fileName.size() + 1);
        fm->offset += fm->length;
        amountRecieved += fm->length;
        remlength -= fm->length;
        // cout <<remlength<<endl;
    }
}

int main(int argc, char *argv[]) {

    start_time=time(NULL);
    signal(SIGALRM, sigFunc);
    alarm(2);

    int z;
    //N
    int n = 100;    //default number of requests per "patient"
    //P
    int p = 10;     // number of patients [1,15]
    //W
    int w = 100;    //default number of worker threads
    //B
    int b = 1024; 	// default capacity of the request buffer, you should change this default
    //H
    int h = 3;      // default number of histogram threads,  you should change this default
    //M
    int m = MAX_MESSAGE; 	// default capacity of the message buffer
    //F
    string filename = "1.csv";
    

    while ((z = getopt(argc, argv, "n:p:w:b:m:f:h:")) != -1){
        switch (z){
            case 'n':
                n = std::atoi(optarg);
            break;
            
            case 'p':
                p = std::atoi(optarg);
            break;
            
            case 'w':
                w = std::atoi(optarg);
            break;
            
            case 'b':
                b = std::atoi(optarg);
            break;
            
            case 'm':
                m = std::atoi(optarg);
                if(m > MAX_MESSAGE){
                    m = MAX_MESSAGE;
                }
            break;
            
            case 'f':
                filename = optarg;
                outFile = true;
            break;
            
            case 'h':
                h = std::atoi(optarg);
            break;
        }
    }

    if(b < 256){
        cerr <<"\n\n\nBuffer capacity is very low and will not be able to get the file name as well!\n\n\n";
        b = 256;
        exit(1);
    }

    srand(time_t(NULL));

    // fork part
    int childID = -1;
    childID = fork();
    if (childID == 0) { // child 
		char* args [] = {"./server", "-m", (char *) to_string(b).c_str(), NULL};
        if (execvp (args [0], args) < 0){
            perror ("exec filed");
            exit (0);
        }
    }
    
    /* Start all threads here */
	FIFORequestChannel* chan = new FIFORequestChannel("control", FIFORequestChannel::CLIENT_SIDE);
    BoundedBuffer requestBuffer(b);
    BoundedBuffer responseBuffer(b);
	
	
    //Clock
    struct timeval start, end;
    gettimeofday(&start,0);

    thread patients [p];
    thread workers [w];
    thread hists [h];
    //Create h histograms and add them to the histogram class!
    for(int i = 0; i < p; i++){
        hc.add(new Histogram(10,-2.0,2.0));
    }
    
    //Create W Channels on Client Side
    FIFORequestChannel *wchans [w];
    //Create w channels
    for(int i = 0; i < w; i++) {
        MESSAGE_TYPE m = NEWCHANNEL_MSG;
        chan->cwrite (&m, sizeof (m));
        char newchanname [100];
        chan->cread (newchanname, sizeof (newchanname));
        wchans[i] = new FIFORequestChannel (newchanname, FIFORequestChannel::CLIENT_SIDE);
    }

    //Create p patient thread!
    if(!outFile){
        for(int i = 0; i < p; i++){
            patients[i] = thread(patient_thread_function, i+1, n, &requestBuffer);
        }
    }
    
    std::thread filethread;
    if(outFile){
        //Create 1 - File Thread
        filethread = std::thread(file_thread_function, filename,&requestBuffer,chan,m);
    }

    //Create w worker thread!
    for(int i = 0; i < w; i++){
        workers[i] = thread(worker_thread_function, &requestBuffer, wchans[i], &responseBuffer, m);
    }

    //Create h histogram thread!
    if(!outFile){
        for(int i = 0; i < h; i++){
            hists[i] = thread(histogram_thread_function, &responseBuffer, &hc);
        }
    }

    /* Join all threads here */
    if(!outFile){
        for(int i = 0; i<p; i++){
            patients[i].join();
        }
    }
    
    if(outFile){
        filethread.join();
    }
    
    //Kill all the worker threads!
    for(int i = 0; i < w; i++){
        MESSAGE_TYPE q = QUIT_MSG;
        requestBuffer.push(&q, sizeof(MESSAGE_TYPE));
    }

    for(int i = 0; i < w; i++){
        workers[i].join();
    }

    Response r{-1,1};
    for(int i = 0; i < h; i++){
        responseBuffer.push(&r, sizeof(r));
    }

    if(!outFile){
        for(int i = 0; i < h; i++){
            hists[i].join();
        }
    }
    
    // print the results
    gettimeofday (&end, 0);
    
    if(!outFile){
        system("clear");
	    hc.print();
    }
    
    cout <<endl;
    cout <<endl;
    int secs = (end.tv_sec * 1e6 + end.tv_usec - start.tv_sec * 1e6 - start.tv_usec)/(int) 1e6;
    int usecs = (int)(end.tv_sec * 1e6 + end.tv_usec - start.tv_sec * 1e6 - start.tv_usec)%((int) 1e6);
    if(!outFile){
        cout << "Took " << secs << " seconds and " << usecs << " micro seconds" << endl;
    }
    else{
        cout << "Took " << secs << " seconds and " << usecs << " micro seconds for FILE DOWNLOAD" << endl;
    }

    MESSAGE_TYPE q = QUIT_MSG;
    chan->cwrite ((char *) &q, sizeof (MESSAGE_TYPE));
    cout << "All Done!!!" << endl;

    wait(0);

    delete chan;
}