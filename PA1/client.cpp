/*
    Harshank Patel
    Department of Computer Science & Engineering
    Texas A&M University
    Date  : 2/12/21
 */
#include "common.h"
#include "FIFOreqchannel.h"
#include <sys/wait.h>

using namespace std;

__int64_t windowSize = MAX_MESSAGE;
int childID = -1;

int main(int argc, char *argv[])
{

    bool isFile = false;
    bool oneDataPoint = false;
    bool oneThousandDataPoint = false;

    int pVal = 0;
    double tVal = 0.00000;
    int eVal = 0;
    string fileName = "";
    bool channelReq = false;

    //Algorithm for the getopt
    int j;
    while ((j = getopt(argc, argv, "p:t:e:f:m:c")) != -1)
    {
        switch (j)
        {
        case 'c':
            channelReq = true;
            std::cout << "New Channel Requested" << '\n';
            break;
        case 'p':
            if (optarg)
            {
                pVal = std::atoi(optarg);
                std::cout << "P val = " << pVal << '\n';
                oneThousandDataPoint = true;
                oneDataPoint = false;
            }
            break;
        case 't':
            if (optarg)
            {
                tVal = std::atof(optarg);
                std::cout << "T val = " << tVal << '\n';
                oneDataPoint = true;
                oneThousandDataPoint = false;
            }
            break;
        case 'e':
            if (optarg)
            {
                eVal = std::atoi(optarg);
                std::cout << "E Val = " << eVal << '\n';
                oneDataPoint = true;
                oneThousandDataPoint = false;
            }
            break;
        case 'f':
            if (optarg)
            {
                fileName = optarg;
                std::cout << "File Name = " << fileName << '\n';
                isFile = true;
            }
            break;
        case 'm':
            windowSize = std::atoi(optarg);
            break;
        }
    }

    //
    /*


    Start the Server!
    */
    childID = fork();
    if (childID == 0)
    {
        //Check for arguments to be passed into Server
        if (windowSize != MAX_MESSAGE)
        {
            cout << "Server Started With a Argument!" << endl;
            string num = to_string(windowSize);
            char char_array[num.length()];
            strcpy(char_array, num.c_str());
            char *argument_list[] = {"./server", "-m", char_array, NULL}; // NULL terminated array of char* strings
            execvp("./server", argument_list);
        }
        else
        {
            cout << "Server Started Without any Arguments!" << endl;
            execvp("./server", NULL);
        }
    }
    else
    {

        /*
        PARENT PROCESS STARTS HERE!
        */

        //Dedault interation channel
        FIFORequestChannel chan("control", FIFORequestChannel::CLIENT_SIDE);

        /*
        These are some of the tasks that we are supposed to do
    */
        if (oneDataPoint)
        {
            cout << "Requesting 1 data point from the server ONLY!" << endl;
            /*    
            cerite corrosponds to writing actual data to the server
        */
            datamsg d(pVal, tVal, eVal);
            chan.cwrite(&d, sizeof(datamsg));

            /*    
            cread basically reads from the server
        */
            double response;
            chan.cread(&response, sizeof(double));
            cout << "For person " << pVal << " , at time " << tVal << " , the value of ech " << eVal << " is " << response << endl;
        }
        else if (isFile)
        {
            cout << "Requesting a file from the server!" << endl;

            //Get the file size of the server
            filemsg f(0, 0);

            //Created a blank buffer using the filemsgg + length of filename + NULL chharacter
            char buf[sizeof(filemsg) + fileName.size() + 1];

            //Copied the file message of type (0,0) to the buffe: buf of size filemsg!!!
            // memcpy(destiination, source, length);
            memcpy(buf, &f, sizeof(filemsg));

            //Copied the name of the file into the buffer: buf of length fileName.size() + 1
            memcpy(buf + sizeof(filemsg), fileName.c_str(), fileName.size() + 1);

            //Now write this buffer into the server of size the buffer +  appeneded fileName
            chan.cwrite(buf, sizeof(buf));

            //Now declare a variable to get the size of file to be stored!
            __int64_t fileLength;

            //read the size of a file from server
            chan.cread(&fileLength, sizeof(__int64_t));
            /*

        GRADER READ THIS:

        First find the remaining of the window by using the mod operator 
        Second then find hhow many iterations you need to perform to get the entire file

        Example: 
        Lets take file size to be 10645
        And your Winidow is: 500


        Step 1: 
        remainingWindow = 10645 % 500
        remainingWindow = 145 LAST WINDOW SIZE

        Step 2:
        numberOfIterations = (10645-145) / 500 = 21 ITERATIONS
        */

            ofstream myfile;
            myfile.open("received/downloaded_" + fileName);

            if (fileLength <= windowSize)
            {
                windowSize = fileLength;

                timeval tim;
                gettimeofday(&tim, NULL);
                double t1 = tim.tv_sec + tim.tv_usec;

                filemsg z(0, windowSize);
                char fileReqBuff[sizeof(filemsg) + fileName.size() + 1];

                memcpy(fileReqBuff, &z, sizeof(filemsg));
                memcpy(fileReqBuff + sizeof(filemsg), fileName.c_str(), fileName.size() + 1);

                chan.cwrite(fileReqBuff, sizeof(fileReqBuff));

                char recievedBuf[windowSize];
                chan.cread(&recievedBuf, sizeof(recievedBuf));

                //use ostream write function!
                myfile.write(recievedBuf, windowSize);

                gettimeofday(&tim, NULL);
                double t2 = tim.tv_sec + tim.tv_usec;
                cout << "Time Taken to get the file of size " << fileLength << " is " << (t2 - t1) << " micro seconds \n\n\n\n\n";
            }
            else
            {
                __int64_t numberOfIterations = 0;
                __int64_t remainingWindow = 0;
                remainingWindow = fileLength % windowSize;
                numberOfIterations = ((fileLength - remainingWindow) / windowSize);

                // cout << numberOfIterations << " is numberOfIterations and this is " << remainingWindow << endl;
                timeval tim;
                gettimeofday(&tim, NULL);
                double t1 = tim.tv_sec + tim.tv_usec;

                for (__int64_t i = 0; i < numberOfIterations; i++)
                {
                    //Algoroithm for recieving the file from the server now!
                    filemsg z(i * windowSize, windowSize);
                    char fileReqBuff[sizeof(filemsg) + fileName.size() + 1];

                    memcpy(fileReqBuff, &z, sizeof(filemsg));
                    memcpy(fileReqBuff + sizeof(filemsg), fileName.c_str(), fileName.size() + 1);

                    chan.cwrite(fileReqBuff, sizeof(fileReqBuff));

                    char recievedBuf[windowSize];
                    chan.cread(&recievedBuf, sizeof(recievedBuf));

                    //use ostream write function!
                    myfile.write(recievedBuf, windowSize);
                }
                if (remainingWindow > 0)
                {
                    filemsg z(numberOfIterations * windowSize, remainingWindow);
                    char fileReqBuff[sizeof(filemsg) + fileName.size() + 1];

                    memcpy(fileReqBuff, &z, sizeof(filemsg));
                    memcpy(fileReqBuff + sizeof(filemsg), fileName.c_str(), fileName.size() + 1);

                    chan.cwrite(fileReqBuff, sizeof(fileReqBuff));

                    char recievedBuf[windowSize];
                    chan.cread(&recievedBuf, sizeof(recievedBuf));

                    myfile.write(recievedBuf, remainingWindow);
                }

                gettimeofday(&tim, NULL);
                double t2 = tim.tv_sec + tim.tv_usec;
                cout << "Time Taken to get the file of size " << fileLength << " is " << (t2 - t1) << " micro seconds \n\n\n\n\n";
            }
            myfile.close();
            cout << "File Copied Successfully!" << endl;
        }
        else if (channelReq)
        {
            MESSAGE_TYPE newChennel = NEWCHANNEL_MSG;
            chan.cwrite(&newChennel, sizeof(MESSAGE_TYPE));

            char newChannelName[30];
            chan.cread(newChannelName, sizeof(newChannelName));

            cout << "New Channel Name Is : " << newChannelName << endl;

            FIFORequestChannel newChan(newChannelName, FIFORequestChannel::CLIENT_SIDE);

            cout << "Requesting 4 data point from the NEW CHANNEL!" << endl;
            datamsg d(1, 0.00, 1);
            newChan.cwrite(&d, sizeof(datamsg));
            double response;
            newChan.cread(&response, sizeof(double));
            cout << "For person " << 1 << " , at time " << 0.00 << " , the value of ech " << 1 << " is " << response << endl;

            datamsg e(2, 0.00, 1);
            newChan.cwrite(&e, sizeof(datamsg));
            newChan.cread(&response, sizeof(double));
            cout << "For person " << 2 << " , at time " << 0.00 << " , the value of ech " << 1 << " is " << response << endl;

            datamsg g(4, 0.00, 1);
            newChan.cwrite(&g, sizeof(datamsg));
            newChan.cread(&response, sizeof(double));
            cout << "For person " << 4 << " , at time " << 0.00 << " , the value of ech " << 1 << " is " << response << endl;

            datamsg h(5, 0.012, 2);
            newChan.cwrite(&h, sizeof(datamsg));
            newChan.cread(&response, sizeof(double));
            cout << "For person " << 5 << " , at time " << 0.012 << " , the value of ech " << 2 << " is " << response << endl;

            MESSAGE_TYPE m = QUIT_MSG;
            newChan.cwrite(&m, sizeof(MESSAGE_TYPE));
        }
        else if (oneThousandDataPoint && !oneDataPoint)
        {
            //Start the time!
            cout << "Requesting 1000 data points from the server!" << endl;
            timeval tim;
            gettimeofday(&tim, NULL);
            double t1 = 1.0e6 * tim.tv_sec + tim.tv_usec;

            ofstream myfile;
            myfile.open("received/x1.csv");

            cout << "Fetching 1000 entries from the server ... \n";
            for (double i = 0; i <= 4; i = i + 0.004)
            {
                datamsg d(pVal, i, 1);
                chan.cwrite(&d, sizeof(datamsg));

                double response1;
                chan.cread(&response1, sizeof(double));

                datamsg x(pVal, i, 2);
                chan.cwrite(&x, sizeof(datamsg));

                double response2;
                chan.cread(&response2, sizeof(double));

                myfile << i << "," << response1 << "," << response2 << endl;
            }
            //End the time!
            gettimeofday(&tim, NULL);
            double t2 = 1.0e6 * tim.tv_sec + tim.tv_usec;

            cout << "Writing 1000 entries to x1.csv file ... \n";
            myfile.close();
            cout << "Done!" << endl;
            cout << "Time Taken to get 1000 points: " << (t2 - t1) << "\n\n\n\n\n";
        }
        else
        {
            // sample that was given to us!
            char buf[MAX_MESSAGE];
            datamsg *x = new datamsg(10, 0.004, 2);

            chan.cwrite(x, sizeof(datamsg));
            int nbytes = chan.cread(buf, MAX_MESSAGE);
            double reply = *(double *)buf;
            cout << "For person 10, at time 0.004, the value of ech 2 is " << reply << "\n\n\n";

            delete x;
        } //Else Block Ends here

        // closing the channel
        MESSAGE_TYPE m = QUIT_MSG;
        chan.cwrite(&m, sizeof(MESSAGE_TYPE));

        //Close the child process
        waitpid(childID, NULL, 0);
        cout << "\n\n\n Now Parent Has Ended" << endl;
    }
}
