#include "common.h"
#include "MQreqchannel.h"
#include <mqueue.h>
using namespace std;

/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR/DESTRUCTOR FOR CLASS   R e q u e s t C h a n n e l  */
/*--------------------------------------------------------------------------*/

MQRequestChannel::MQRequestChannel(const string _name, const Side _side) : RequestChannel(_name,_side){
	s1 = "/MQ_" + my_name + "1";
	s2 = "/MQ_" + my_name + "2";
		
	if (_side == SERVER_SIDE){
		wfd = open_ipc(s1, O_RDWR|O_CREAT);
		rfd = open_ipc(s2, O_RDWR|O_CREAT);
	}

	else{
		rfd = open_ipc(s1, O_RDWR|O_CREAT);
		wfd = open_ipc(s2, O_RDWR|O_CREAT);
	}
}

MQRequestChannel::~MQRequestChannel(){ 
	mq_close (wfd);
	mq_close (rfd);

	if(mq_unlink (s1.c_str()) == 0) {
		// cout <<"Close it! "<<s1.c_str() <<endl;
	}
	else{
		// cout <<"DIDNT Close it! "<<s1.c_str() <<endl;
	}
	if(mq_unlink (s2.c_str()) == 0) {
		// cout <<"Close it! "<<s2.c_str() <<endl;
	}
	else{
		// cout <<"DIDNT Close it! "<<s2.c_str() <<endl;
	}
}



int MQRequestChannel::open_ipc(string _pipe_name, int mode){
	
	// struct mq_attr {
    //            long mq_flags;       /* Flags: 0 or O_NONBLOCK */
    //            long mq_maxmsg;      /* Max. # of messages on queue */
    //            long mq_msgsize;     /* Max. message size (bytes) */
    //            long mq_curmsgs;     /* # of messages currently in queue */
    //        };

	struct mq_attr mqAttr;
	mqAttr.mq_maxmsg = 10; //Max Value
	if(MAX_MESSAGE > 256) {
		mqAttr.mq_msgsize = 256; // Max Valuue
	}
	else {
		mqAttr.mq_msgsize = MAX_MESSAGE; // Max Valuue
	}

	int fd = (int) mq_open (_pipe_name.c_str(), mode, 0600, &mqAttr);
	// int fd = (int) mq_open (_pipe_name.c_str(), mode, 0600,0);

	if (fd < 0){
		EXITONERROR(_pipe_name);
	}
	return fd;
}

int MQRequestChannel::cread(void* msgbuf, int bufcapacity){
	//Default value for 8K hard coded in here!
	return mq_receive (rfd, (char *) msgbuf, 8192, NULL);
}

int MQRequestChannel::cwrite(void* msgbuf, int len){
	return mq_send (wfd, (char *)msgbuf, len, 0);
}

