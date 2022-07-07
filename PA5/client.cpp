#include "common.h"
#include <sys/wait.h>
#include "FIFOreqchannel.h"
#include "BoundedBuffer.h"
#include "Histogram.h"
#include "HistogramCollection.h"
#include <time.h>
#include <thread>
#include <sys/epoll.h>
#include <fcntl.h>
#include <vector>
#include <unordered_map>
using namespace std;

struct Response{
    int person;
    double ecgval;

    Response (int _p, double _e): person (_p), ecgval (_e){;}

};
void timediff (struct timeval& start, struct timeval& end){
    int secs = (end.tv_sec * 1e6 + end.tv_usec - start.tv_sec * 1e6 - start.tv_usec)/(int) 1e6;
    int usecs = (int)(end.tv_sec * 1e6 + end.tv_usec - start.tv_sec * 1e6 - start.tv_usec)%((int) 1e6);
    cout << "Took " << secs << " seconds and " << usecs << " micro seconds" << endl;
}
FIFORequestChannel* create_new_channel (FIFORequestChannel* mainchan){
    char name [1024];
    MESSAGE_TYPE m = NEWCHANNEL_MSG;
    mainchan->cwrite (&m, sizeof (m));
    mainchan->cread (name, 1024);
    FIFORequestChannel* newchan = new FIFORequestChannel (name, FIFORequestChannel::CLIENT_SIDE);
    return newchan;
}
void patient_thread_function (int n, int p, BoundedBuffer* reqbuffer){
    double t = 0;
    datamsg d (p, t, 1);
    for (int i=0; i<n; i++){
        reqbuffer->push ((char*) &d, sizeof (d));
        d.seconds += 0.004;
    }
    
}
void worker_thread_function (int mb, BoundedBuffer* requestBuffer, FIFORequestChannel* workChan, BoundedBuffer* responseBuffer){
    char buf [1024];
    double resp = 0;

    char recvbuf [mb];
    while (true){
        requestBuffer->pop (buf, 1024);
        MESSAGE_TYPE* m = (MESSAGE_TYPE *) buf;
        
        if (*m == DATA_MSG){
            datamsg* d = (datamsg*) buf;
            workChan->cwrite (d, sizeof (datamsg));
            workChan->cread (&resp, sizeof (double));
            Response r (d->person, resp);
            responseBuffer->push ((char*)&r, sizeof (Response));
        }else if (*m == QUIT_MSG){
            workChan->cwrite (m, sizeof (MESSAGE_TYPE));
            delete workChan;
            break;
        }else if (*m == FILE_MSG){
            filemsg* fm = (filemsg*) buf;
            string fname = (char *)(fm + 1);
            int sz = sizeof (filemsg) + fname.size() + 1;
            workChan->cwrite (buf, sz);
            workChan->cread (recvbuf, mb);

            string recvfname = "recv/" + fname;
            FILE* fp = fopen (recvfname.c_str(), "r+");
            fseek (fp, fm->offset, SEEK_SET);
            fwrite (recvbuf, 1, fm->length, fp);
            fclose (fp);
        }
    }
}
void histogram_thread_function (BoundedBuffer* responseBuffer, HistogramCollection* hc){
    char buf [1024];
    Response* r = (Response *) buf;
    while (true){
        responseBuffer->pop (buf, 1024);
        if (r->person < 1){ // it means quit
            break;
        }
        hc->update (r->person, r->ecgval);
    }
}
void epoll_thread_function(int w, FIFORequestChannel ** wchans, BoundedBuffer* request_buffer, HistogramCollection* hc, int mb, BoundedBuffer * response_buffer){
    char buf[1024];
    double resp = 0;
    char recvbuf[mb];

    struct epoll_event ev;
    struct epoll_event events[w];

    int epollfd = epoll_create1(0);
    if(epollfd == -1){
        EXITONERROR("epoll_create1");
    }


    unordered_map<int,int> fd_to_index;
    vector<vector<char>> state(w);

    int nrecv = 0;
    int nsent = 0;
    for(int i=0;i<w;i++){
        int sz=request_buffer->pop(buf,1024);
        wchans[i]->cwrite(buf, sz);
        nsent++;
        state[i] = vector<char> (buf, buf+sz);

        int rfd = wchans[i]->getrfd();
        fcntl(rfd, F_SETFL, O_NONBLOCK);

        ev.events = EPOLLIN | EPOLLET;
        ev.data.fd = rfd;
        fd_to_index[rfd] = i;
        if(epoll_ctl(epollfd, EPOLL_CTL_ADD, rfd, &ev) == -1){
            EXITONERROR("epoll_ctl");
        }
    }
    bool quit_recieved = false;
    while(true){
        if(quit_recieved == true && nrecv == nsent){
            break;
        }

        int nfds = epoll_wait(epollfd, events,w,-1);
        if(nfds == -1){
            EXITONERROR("epoll_wait");
        }

        for(int i = 0; i < nfds; i++){
            int rfd = events[i].data.fd; //100, 109
            int index = fd_to_index[rfd];
            int resp = wchans[index]->cread(recvbuf,mb);
            nrecv++;
            // cerr <<" i here is = "<<i <<endl;
            vector<char> req = state[index];
            char* request = req.data();
            MESSAGE_TYPE *m = (MESSAGE_TYPE *) request;
            if (*m == DATA_MSG){
                Response r{((datamsg *) request)->person, *(double*)recvbuf};
                response_buffer->push((char *)&r, sizeof(Response));
            }
            else if (*m == FILE_MSG){
                filemsg *fm = (filemsg*)request;
                string fname = (char *)(fm+1);
                int sz = sizeof(filemsg) + fname.size()+1;
                string recvfname = "recieved/"+fname;
                
                FILE * fp = fopen(recvfname.c_str(),"r+");
                fseek(fp, fm->offset, SEEK_SET);
                fwrite(recvbuf, 1, fm->length,fp);
                fclose(fp);
            }
            if(quit_recieved == false){
                int req_sz = request_buffer->pop(buf, sizeof(buf));
                wchans[index]->cwrite(buf, req_sz);
                state[index] = vector<char> (buf, buf+req_sz);
                nsent++;
                if(*(MESSAGE_TYPE*) buf == QUIT_MSG){
                    quit_recieved = true;
                    
                }
            }
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
    while(remlength > 0){
        fm->length = min(remlength, (__int64_t) mb);
        requestBuffer->push(buf,sizeof(filemsg) + fileName.size() + 1);
        fm->offset += fm->length;
        remlength -= fm->length;
    }
}


int main(int argc, char *argv[]){
    int c;
    int buffercap = MAX_MESSAGE;
    int p = 10, ecg = 1;
    double t = -1.0;
    bool isnewchan = false;
    bool isfiletransfer = false;
    string filename;
    int b = 1024;
    int w = 100;
    int n = 10000;
    int m = MAX_MESSAGE;
    int h = 3;


    while ((c = getopt (argc, argv, "p:t:e:m:f:b:c:w:n:h:")) != -1){
        switch (c){
            case 'p':
                p = atoi (optarg);
                break;
            case 't':
                t = atof (optarg);
                break;
            case 'e':
                ecg = atoi (optarg);
                break;
            case 'm':
                buffercap = atoi (optarg);
                m = buffercap;
                break;
            case 'c':
                isnewchan = true;
                break;
            case 'f':
                isfiletransfer = true;
                filename = optarg;
                break;
            case 'b':
                b = atoi (optarg);
                break;
            case 'w':
                w = atoi (optarg);
                break;
            case 'n':
                n = atoi (optarg);
                break;
            case 'h':
                h = atoi (optarg);
                break;
        }
    }
    if(w > n*p){
        w = n * p-1;
    }
    
    // fork part
    if (fork()==0){ // child 
	
		char* args [] = {"./server", "-m", (char *) to_string(buffercap).c_str(), NULL};
        if (execvp (args [0], args) < 0){
            perror ("exec filed");
            exit (0);
        }
    }

    FIFORequestChannel* chan = new FIFORequestChannel("control", FIFORequestChannel::CLIENT_SIDE);
    BoundedBuffer requestBuffer (b);
	BoundedBuffer responseBuffer (b);
    HistogramCollection hc;

    // making histograms and adding to the histogram collection hc
    for (int i=0; i<p; i++){
        Histogram* h = new Histogram (10, -2.0, 2.0);
        hc.add (h);
    }

    // make w worker channels (make sure to do it sequentially in the main)
    FIFORequestChannel** wchans = new FIFORequestChannel *[w];
    for (int i=0; i<w; i++){
        wchans [i] = create_new_channel (chan);     
    }
	

    thread filethread;
    if(isfiletransfer){
        cout << "File threads Created" << endl;
        filethread = thread(file_thread_function, filename,&requestBuffer,chan,m);
    }
    
    struct timeval start, end;
    gettimeofday (&start, 0);
    thread patient [p];
	if(!isfiletransfer){
        cout << "Patient threads Created" << endl;
        for (int i=0; i<p; i++){
            patient [i] = thread (patient_thread_function, n, i+1, &requestBuffer);
        }
    }
    

    thread evp (epoll_thread_function, w, wchans, &requestBuffer, &hc, m, &responseBuffer);
    
    

    thread hists [h];
    for (int i=0; i<h; i++){
        hists [i] = thread (histogram_thread_function, &responseBuffer, &hc);
    }
    
  	/* Join all threads here */
    if(!isfiletransfer){
        for (int i=0; i<p; i++){
            patient [i].join ();
        }
        cout << "Patient threads finished" << endl;
    }
    

    if(isfiletransfer){
        filethread.join();
        cout << "File threads finished" << endl;
    }


    MESSAGE_TYPE quits = QUIT_MSG;
    requestBuffer.push ((char*) &quits, sizeof (quits));

    evp.join();

    cout << "EVP threads finished" << endl;

    for (int i=0; i<h; i++){
        datamsg d(-1, 0, -1);
        responseBuffer.push ((char*)&d, sizeof (d));
    }
    for (int i=0; i<h; i++){
        hists [i].join ();
    }
    cout << "Histogram threads are done. All client threads are now done" << endl;

    gettimeofday (&end, 0);
    // print time difference
    timediff (start, end);
    

    // print the results
    if(!isfiletransfer){
        hc.print ();
    }

    //Free Up the Stupid WChans!
    for(int i = 0; i < w; ++i) {
        delete wchans[i];
    }
    delete[] wchans;
    
    // cleaning the main channel
    MESSAGE_TYPE q = QUIT_MSG;
    chan->cwrite ((char *) &q, sizeof (MESSAGE_TYPE));
    wait (0);
    cout << "All Done!!!" << endl;
    delete chan;
}
