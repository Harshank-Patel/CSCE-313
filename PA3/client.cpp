/*
    Harshank Patel
    Department of Computer Science & Engineering
    Texas A&M University
    Date  : 03/16/21
 */
#include "common.h"
#include "SHMreqchannel.h"
#include <sys/wait.h>
#include "FIFOreqchannel.h"
#include "MQreqchannel.h"


using namespace std;

//If I have lets saw 5 channels, how will I get the code to work for 1000 requests with 5 channels?
//Answer: 1000 / 5 = 200 so every 200 data points, I will be changing my channel!

RequestChannel* generateChannel(RequestChannel*control_chan,string type,int buffercap){
    MESSAGE_TYPE m = NEWCHANNEL_MSG;
    control_chan->cwrite (&m, sizeof (m));
    char newchanname [100];
    control_chan->cread (newchanname, sizeof (newchanname));
    if(type == "f") {
        return  new FIFORequestChannel (newchanname, RequestChannel::CLIENT_SIDE);
    }
    else if(type == "q") {
        return  new MQRequestChannel (newchanname, RequestChannel::CLIENT_SIDE);
    }
    else if(type == "m") {
        return  new SHMRequestChannel (newchanname, RequestChannel::CLIENT_SIDE,buffercap);
    }
    return nullptr;
}


int main(int argc, char *argv[]){
    
    int c;
    int buffercap = MAX_MESSAGE;
    int p = 0, ecg = 1;
    double t = -1.0;
    bool isnewchan = false;
    bool isfiletransfer = false;
    string filename;
    string ipcmethod = "f";
    int nchannels = 1;


    while ((c = getopt (argc, argv, "p:t:e:m:f:c:i:")) != -1){
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
                break;
            case 'c':
                isnewchan = true;
                nchannels = atoi(optarg);
                break;
            case 'f':
                isfiletransfer = true;
                filename = optarg;
                break;
            case 'i':
                ipcmethod = optarg;
                break;
        }
    }
    
    // fork part
    int childID = -1;
    childID = fork();
    if (childID == 0) { // child 
		char* args [] = {"./server", "-m", (char *) to_string(buffercap).c_str(),"-i",(char *)ipcmethod.c_str(), NULL};
        if (execvp (args [0], args) < 0){
            perror ("exec filed");
            exit (0);
        }
    }


    RequestChannel* control_chan = NULL;
    if(ipcmethod == "f"){
        control_chan = new FIFORequestChannel ("control", RequestChannel::CLIENT_SIDE);
    }
    else if(ipcmethod == "q"){
        control_chan = new MQRequestChannel ("control", RequestChannel::CLIENT_SIDE);
    }
    else if(ipcmethod == "m"){
        control_chan = new SHMRequestChannel ("control", RequestChannel::CLIENT_SIDE, buffercap);
    }


    //Create multiple channels!
    RequestChannel* Array_Of_Channels[nchannels];
    if(nchannels > 1){
        for(int i = 0; i < nchannels; i++) {
            if(ipcmethod == "f") {
                Array_Of_Channels[i] = generateChannel(control_chan,"f",buffercap);
            }
            else if(ipcmethod == "q") {
                Array_Of_Channels[i] = generateChannel(control_chan,"q",buffercap);
            }
            else if(ipcmethod == "m") {
                Array_Of_Channels[i] = generateChannel(control_chan,"m",buffercap);
            }
        }
    }



    RequestChannel* chan = control_chan;
    if (isnewchan){
        cout << "Using the new channel everything following" << endl;
        MESSAGE_TYPE m = NEWCHANNEL_MSG;
        control_chan->cwrite (&m, sizeof (m));
        char newchanname [100];
        control_chan->cread (newchanname, sizeof (newchanname));
        if(ipcmethod == "f"){
            chan = new FIFORequestChannel (newchanname, RequestChannel::CLIENT_SIDE);
        }
        else if(ipcmethod == "q"){
            chan = new MQRequestChannel (newchanname, RequestChannel::CLIENT_SIDE);
        }
        else if(ipcmethod == "m"){
            chan = new SHMRequestChannel (newchanname, RequestChannel::CLIENT_SIDE,buffercap);
        }
        
        cout << "New channel by the name " << newchanname << " is created" << endl;
        cout << "All further communication will happen through it instead of the main channel" << endl;
    }

    if (!isfiletransfer){   // requesting data msgs
        //Start the time!
        cout << "Requesting 1000 data points from the server!" << endl;
        timeval tim;
        gettimeofday(&tim, NULL);
        double t1 = 1.0e6 * tim.tv_sec + tim.tv_usec;

        if (t >= 0){    // 1 data point
            datamsg d (p, t, ecg);
            chan->cwrite (&d, sizeof (d));
            double ecgvalue;
            chan->cread (&ecgvalue, sizeof (double));
            cout << "Ecg " << ecg << " value for patient "<< p << " at time " << t << " is: " << ecgvalue << endl;
        } else{          // bulk (i.e., 1K) data requests 
            double ts = 0;  
            datamsg d (p, ts, ecg);
            double ecgvalue;

            //Now Bifercate the algorithm for the Number of Channels!
            if(nchannels > 1) {
                if(1000 % nchannels > 0) {
                    
                    //Perform thhe divisions in using the floor option
                    //And then whatever mod is left, use the Arr[0] channel to get it done!
                    int remainingValues = 1000 % nchannels;
                    int numDivisions = (1000/nchannels);
                    int iterator = 0;
                    int pos = 0;
                    for (int i=0; i<1000-remainingValues; i++){
                        if(iterator == numDivisions){
                            pos = pos + 1;
                            iterator = 0;
                            cout <<"Channel Changed to ..." <<Array_Of_Channels[pos]->name() <<endl;
                        }
                        Array_Of_Channels[pos]->cwrite (&d, sizeof (d));
                        Array_Of_Channels[pos]->cread (&ecgvalue, sizeof (double));
                        d.seconds += 0.004; //increment the timestamp by 4ms
                        cout << ecgvalue << endl;
                        iterator++;
                    }
                    cout <<"Channel Changed to ..." <<Array_Of_Channels[0]->name() <<endl;;
                    for (int i=1000-remainingValues; i<1000; i++){
                        Array_Of_Channels[0]->cwrite (&d, sizeof (d));
                        Array_Of_Channels[0]->cread (&ecgvalue, sizeof (double));
                        d.seconds += 0.004; //increment the timestamp by 4ms
                        cout << ecgvalue << endl;
                    }
                    
                }
                else{
                    //Perform the divisions based on the divisions only!
                    int numDivisions = (1000/nchannels);
                    int iterator = 0;
                    int pos = 0;
                    for (int i=0; i<1000; i++){
                        if(iterator == numDivisions){
                            pos = pos + 1;
                            iterator = 0;
                            cout <<"Channel Changed to ..." <<Array_Of_Channels[pos]->name() <<endl;;
                        }
                        Array_Of_Channels[pos]->cwrite (&d, sizeof (d));
                        Array_Of_Channels[pos]->cread (&ecgvalue, sizeof (double));
                        d.seconds += 0.004; //increment the timestamp by 4ms
                        cout << ecgvalue << endl;
                        iterator++;
                    }
                }
            }
            else {
                for (int i=0; i<1000; i++){
                    chan->cwrite (&d, sizeof (d));
                    chan->cread (&ecgvalue, sizeof (double));
                    d.seconds += 0.004; //increment the timestamp by 4ms
                    cout << ecgvalue << endl;
                }
            }
        }
        //End the time!
        gettimeofday(&tim, NULL);
        double t2 = 1.0e6 * tim.tv_sec + tim.tv_usec;
        cout << "Time Taken to get 1000 points: " << (t2 - t1) << "\n\n\n\n\n";
    }
    
    else if (isfiletransfer) {
        timeval tim;
        gettimeofday(&tim, NULL);
        double t1 = 1.0e6 * tim.tv_sec + tim.tv_usec;

        cout << "Requesting a file from the server!" << endl;
        //Get the file size of the server
        filemsg f(0, 0);
        //Created a blank buffer using the filemsgg + length of filename + NULL chharacter
        char buf[sizeof(filemsg) + filename.size() + 1];
        //Copied the file message of type (0,0) to the buffe: buf of size filemsg!!!
        // memcpy(destiination, source, length);
        memcpy(buf, &f, sizeof(filemsg));
        //Copied the name of the file into the buffer: buf of length filename.size() + 1
        memcpy(buf + sizeof(filemsg), filename.c_str(), filename.size() + 1);
        //Now write this buffer into the server of size the buffer +  appeneded filename
        chan->cwrite(buf, sizeof(buf));
        //Now declare a variable to get the size of file to be stored!
        __int64_t fileLength;
        //read the size of a file from server
        chan->cread(&fileLength, sizeof(__int64_t));


        ofstream myfile;
        myfile.open("received/downloaded_" + filename);
        if (fileLength <= buffercap) {
            buffercap = fileLength;


            RequestChannel* tempChannel = chan;
            if(nchannels > 1){
                chan = Array_Of_Channels[0];
            }
            // timeval tim;
            // gettimeofday(&tim, NULL);
            // double t1 = tim.tv_sec + tim.tv_usec;

            filemsg z(0, buffercap);
            char fileReqBuff[sizeof(filemsg) + filename.size() + 1];

            memcpy(fileReqBuff, &z, sizeof(filemsg));
            memcpy(fileReqBuff + sizeof(filemsg), filename.c_str(), filename.size() + 1);

            chan->cwrite(fileReqBuff, sizeof(fileReqBuff));

            char recievedBuf[buffercap];
            chan->cread(&recievedBuf, sizeof(recievedBuf));

            //use ostream write function!
            myfile.write(recievedBuf, buffercap);

            //Lets revert it back to the original channel!
            if(nchannels > 1){
                chan = tempChannel;
            }
            tempChannel = nullptr;

        }
        else if(nchannels > 1){

            __int64_t numberOfIterations = 0;
            __int64_t remainingWindow = 0;
            remainingWindow = fileLength % buffercap;
            numberOfIterations = ((fileLength - remainingWindow) / buffercap);
            cout <<"\n\nRemaining Bytes is: "<<remainingWindow<<"\n";
            cout <<"Number of Iterattions To Use is: "<<numberOfIterations << " of bufferSize : " <<buffercap <<"\n";
            cout <<"The Number of Channels To Use is: "<<nchannels <<"\n\n";
            
            // timeval tim;
            // gettimeofday(&tim, NULL);
            // double t1 = tim.tv_sec + tim.tv_usec;
            int pos = 0;
            for (__int64_t i = 0; i < numberOfIterations; i++) {
                //Algoroithm for recieving the file from the server now!

                if(pos+1 >= nchannels){
                    pos = 0;
                }
                else{
                    pos+=1;
                }
                cout <<"Got Buffer Size ["<<i*buffercap <<", "<<(i*buffercap+buffercap) <<"] with Index :" <<pos <<"\n";

                filemsg z(i * buffercap, buffercap);
                char fileReqBuff[sizeof(filemsg) + filename.size() + 1];

                memcpy(fileReqBuff, &z, sizeof(filemsg));
                memcpy(fileReqBuff + sizeof(filemsg), filename.c_str(), filename.size() + 1);

                Array_Of_Channels[pos]->cwrite(fileReqBuff, sizeof(fileReqBuff));

                char recievedBuf[buffercap];
                Array_Of_Channels[pos]->cread(&recievedBuf, sizeof(recievedBuf));

                //use ostream write function!
                myfile.write(recievedBuf, buffercap);
            }
            if (remainingWindow > 0) {
                if(pos+1 >= nchannels){
                    pos = 0;
                }
                else{
                    pos+=1;
                }
                cout <<"Got Buffer Size ["<<(numberOfIterations * buffercap) <<", "<<(numberOfIterations * buffercap + remainingWindow) <<"] with Index :" <<pos <<"\n";

                filemsg z(numberOfIterations * buffercap, remainingWindow);
                char fileReqBuff[sizeof(filemsg) + filename.size() + 1];

                memcpy(fileReqBuff, &z, sizeof(filemsg));
                memcpy(fileReqBuff + sizeof(filemsg), filename.c_str(), filename.size() + 1);

                Array_Of_Channels[pos]->cwrite(fileReqBuff, sizeof(fileReqBuff));

                char recievedBuf[buffercap];
                Array_Of_Channels[pos]->cread(&recievedBuf, sizeof(recievedBuf));

                myfile.write(recievedBuf, remainingWindow);
            }

            // gettimeofday(&tim, NULL);
            // double t2 = tim.tv_sec + tim.tv_usec;
            // cout << "Time Taken to get the file of size " << fileLength << " is " << (t2 - t1) << " micro seconds \n\n\n\n\n";
        }
        else {
            __int64_t numberOfIterations = 0;
            __int64_t remainingWindow = 0;
            remainingWindow = fileLength % buffercap;
            numberOfIterations = ((fileLength - remainingWindow) / buffercap);
            
            // timeval tim;
            // gettimeofday(&tim, NULL);
            // double t1 = tim.tv_sec + tim.tv_usec;

            for (__int64_t i = 0; i < numberOfIterations; i++) {
                //Algoroithm for recieving the file from the server now!
                filemsg z(i * buffercap, buffercap);
                char fileReqBuff[sizeof(filemsg) + filename.size() + 1];

                memcpy(fileReqBuff, &z, sizeof(filemsg));
                memcpy(fileReqBuff + sizeof(filemsg), filename.c_str(), filename.size() + 1);

                chan->cwrite(fileReqBuff, sizeof(fileReqBuff));

                char recievedBuf[buffercap];
                chan->cread(&recievedBuf, sizeof(recievedBuf));

                //use ostream write function!
                myfile.write(recievedBuf, buffercap);
            }

            if (remainingWindow > 0) {
                filemsg z(numberOfIterations * buffercap, remainingWindow);
                char fileReqBuff[sizeof(filemsg) + filename.size() + 1];

                memcpy(fileReqBuff, &z, sizeof(filemsg));
                memcpy(fileReqBuff + sizeof(filemsg), filename.c_str(), filename.size() + 1);

                chan->cwrite(fileReqBuff, sizeof(fileReqBuff));

                char recievedBuf[buffercap];
                chan->cread(&recievedBuf, sizeof(recievedBuf));

                myfile.write(recievedBuf, remainingWindow);
            }

            // gettimeofday(&tim, NULL);
            // double t2 = tim.tv_sec + tim.tv_usec;
            // cout << "Time Taken to get the file of size " << fileLength << " is " << (t2 - t1) << " micro seconds \n\n\n\n\n";
        }
        

        //End the time!
        gettimeofday(&tim, NULL);
        double t2 = 1.0e6 * tim.tv_sec + tim.tv_usec;
        cout << "Time Taken to get the file is: " << (t2 - t1) << "\n\n\n\n\n";

        myfile.close();
        cout << "File Copied Successfully!" << endl;
    }


    MESSAGE_TYPE q = QUIT_MSG;
    if(nchannels > 1) {
        for(int i = 0 ; i < nchannels; i++) {
            Array_Of_Channels[i]->cwrite (&q, sizeof (MESSAGE_TYPE));
            delete Array_Of_Channels[i];
        }
    }

    if(control_chan != chan){
        //Only Two!
        MESSAGE_TYPE q = QUIT_MSG;
        control_chan->cwrite (&q, sizeof (MESSAGE_TYPE));
        chan->cwrite (&q, sizeof (MESSAGE_TYPE));
        delete control_chan;
        delete chan;
    }
    else{
        //Only One!
        MESSAGE_TYPE q = QUIT_MSG;
        chan->cwrite (&q, sizeof (MESSAGE_TYPE));
        delete chan;
    }

    
    cout << "Client-side is done and exited" << endl;



	// wait for the child process running server
    // this will allow the server to properly do clean up
    // if wait is not used, the server may sometimes crash
	waitpid(childID, NULL, 0);
    cout << "Now Parent Has Ended" << endl;
}
