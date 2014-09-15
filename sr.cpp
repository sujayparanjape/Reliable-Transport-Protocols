#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <cstring>
#include <vector>
#include <sstream>
#include <queue>
#include "sr_CustomVariables.h"
#include<cmath>
#include <iostream>
#include <fstream>


using namespace std;
/* ******************************************************************
 ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.1  J.F.Kurose

   This code should be used for PA2, unidirectional or bidirectional
   data transfer protocols (from A to B. Bidirectional transfer of data
   is for extra credit and is not required).  Network properties:
   - one way network delay averages five time units (longer if there
     are other messages in the channel for GBN), but can be larger
   - packets can be corrupted (either the header or the data portion)
     or lost, according to user-defined probabilities
   - packets will be delivered in the order in which they were sent
     (although some can be lost).
**********************************************************************/

#define BIDIRECTIONAL 0    /* change to 1 if you're doing extra credit */
                           /* and write a routine called B_output */

/* a "msg" is the data unit passed from layer 5 (teachers code) to layer  */
/* 4 (students' code).  It contains the data (characters) to be delivered */
/* to layer 5 via the students transport level protocol entities.         */
struct msg {
  char data[20];
  };

/* a packet is the data unit passed from layer 4 (students code) to layer */
/* 3 (teachers code).  Note the pre-defined packet structure, which all   */
/* students must follow. */
struct pkt {
   int seqnum;
   int acknum;
   int checksum;
   char payload[20];
    };

void tolayer3(int,pkt);
void tolayer5(int,char *);
void stoptimer(int AorB);
void starttimer(int,float);
extern float time_local;
void moveSndrWindow();
void moveRecvWindow();
void printmsg(string msg, int status, int callingEntity);

struct pktEncap
{
 struct pkt datapkt;
 bool isAcked;
 bool isRecved;
 bool isDelivered;
 bool isRetransmitted;
 float transmissionTime;
 };


stringstream ss_msg;

struct queueElem
{
	float timeOut;
	int seqNo;
	long insertionSeq;

	bool operator<(const queueElem &other) const
	{
		if(timeOut < other.timeOut)
			return false;
		else if (timeOut > other.timeOut)
			return true;
		else if (insertionSeq < other.insertionSeq)
			{
/*		if(seqNo > other.seqNo)
			{ 
				printf("Same timeout for packet %d insertionSeq: %ld and %d insertionSeq: %ld at %.6f 		\n",seqNo,insertionSeq,other.seqNo,other.insertionSeq,timeOut);
			}
*/
			return false;
			}	
		else
			{

	/*		if(seqNo < other.seqNo)
			{
				printf("Same timeout for packet %d insertionSeq: %ld and %d insertionSeq: %ld at  %.6f\n",seqNo,insertionSeq,other.seqNo,other.insertionSeq,timeOut);
			}
*/
			return true;
			}	
	}
};


/******** STATIC VARIABLES *********/

struct pkt recvpkt;
//float const timout_a = 20.0;
string const protocolName = "Selective Repeat";
int pktCount_snd_transport_A = 0;
int pktCount_rcv_transport_B = 0;
int pktCount_rcv_appln_B = 0;
long globleInsertionSeq = 0;
float estimatedRTT;
float estimatedTimeout;
float devRTT;

//sender buffer
vector<pktEncap> sndBuffer;

//receiver buffer
 struct pktEncap recvBuffer[WINDOWSIZE];

//sender buffer capacity
int bufferCapacity_sndr;

int bufferCapacity_recv;

int sendBase, nextSeqNumber;

int recvBase;
int recvBaseBuffPos;
//priority queue that gives next packet whose timeout is to be executed.
priority_queue< queueElem, vector<queueElem> , less<queueElem> > timerQueue;

//packets yet to be sent.
int pendingCount;

//int nextExpectedSeqNo_recv;



/********* HELPER ROUTINES *********/
/*
 * Calculate the checksum and return the value of checksum
 * @packetPtr : pointer to a packet whose checksum need to be calculated.
 */
int getCheckSum(struct pkt * packetPtr)
{
 int checksum = packetPtr->seqnum +packetPtr->acknum;
 for (int j = 0 ; j< sizeof packetPtr->payload; j ++)
 {
	 checksum = checksum + packetPtr->payload[j];
 }
 return checksum;
}

/*
 *  print the message.
 *  @msg: message to be print
 *  @status: 0=> Info message 1=> Error message
 *  @callingEntity : 0 => A , 1=> B
 */

void printmsg(string msg, int status, int callingEntity)
{
	int emptySpace = callingEntity == 0 ? 1: 30;

	if(status == 0)
	{if(enableMsgLog)
	{
		cout<< string(emptySpace, ' ')<<msg<<endl;
		cout.flush();
	}
	}
	else
	{
		cout<<string(emptySpace, ' ')<<"ErrorMessage: "<<msg<<endl;
	}
}


/* function is used to print results to output file which can be used for performance comparison */
void storeValue(float time, int nsim)
{
	extern int nsimmax;
	extern float time_local;
	extern float lossprob;            /* probability that a packet is dropped  */
	extern float corruptprob;         /* probability that one bit is packet is flipped */
	extern float lambda;              /* arrival rate of messages from layer 5 */

	  ofstream myfile;
	  myfile.open ("output.txt",ios::app);
	  myfile << protocolName<<"^"
			 << nsim<<"^"
			 << pktCount_snd_transport_A<<"^"
			 << pktCount_rcv_transport_B<<"^"
			 << pktCount_rcv_appln_B<<"^"
			 << time<<"^"
			 << (float)pktCount_rcv_appln_B / time<<"^"
			 << nsimmax<<"^"
			 << lossprob<<"^"
			 << corruptprob<<"^"
			 << lambda<<"^"
			 << WINDOWSIZE<<"\n";
	  myfile.close();
}

/*
 * This function prints the simulation results.
 * @time: total time elapsed
 * @nsim: no of messages simulated.
 */
void printResult(float time, int nsim)
{
	cout<<"Protocol: "<<protocolName<<endl;
	cout<< nsim <<" of packets sent from the Application Layer of Sender A"<<endl;
	cout<< pktCount_snd_transport_A<<" of packets sent from the Transport Layer of Sender A"<<endl;
	cout<< pktCount_rcv_transport_B <<" packets received at the Transport layer of Receiver B" <<endl;
	cout<< pktCount_rcv_appln_B <<" of packets received at the Application layer of Receiver B"<<endl;
	cout<<"Total time: "<< time <<" time units"<<endl;
	printf("Throughput = %.6f packets/time units \n",(float)pktCount_rcv_appln_B / time);
}

/*function returns adaptive timeout value if  isAdaptiveRTT is set to 1 in sr_CustomVariable.h*/
float getTimeOut()
{
	return isAdaptiveRTT ? estimatedTimeout:timeoutVal;
	//return estimatedTimeout;
	//return timeoutVal;
}


void addToSndBuffer(struct pktEncap newPkt)
{
	// check remaining capacity
	if(bufferCapacity_sndr <= 0)
	{
		printmsg("Buffer capacity exceeded. Exiting the program",1,0);
		exit(0);
	}

	//put the packet at back
	sndBuffer.push_back(newPkt);

	//decrease the remaining capacity
	bufferCapacity_sndr--;

}

void addToRecvBuffer(struct pktEncap newPkt)
{
//	// check remaining capacity
//	if(bufferCapacity_recv <= 0)
//	{
//		printmsg("Buffer capacity exceeded at the receiver. Exiting the program",1,1);
//		exit(0);
//	}

	int recvBaseRelativePos =(newPkt.datapkt.seqnum + availableSeqNos-recvBase)%availableSeqNos;


	if((recvBaseRelativePos + 1) >(sizeof recvBuffer/  sizeof(pktEncap)))
		{
			printmsg("Buffer capacity exceeded at the receiver. Exiting the program",1,1);
			exit(0);
		}

	recvBuffer[(recvBaseBuffPos+ recvBaseRelativePos)%(sizeof recvBuffer/  sizeof(pktEncap))] = newPkt;

	//decrease the remaining capacity
	bufferCapacity_recv--;
}


int isOutofWindow(int ackNo, int sendBase, int nextSeqNo)
{
	int end1;
	int end2;
	end1 = nextSeqNo;//(sendBase + WINDOWSIZE) % availableSeqNos;
	end2 = (sendBase + availableSeqNos -1 ) % availableSeqNos;
	stringstream ss ;
	ss<<"Ack no to check is ";
	ss<<ackNo;
	ss<<". Non window range : ";
	ss<<end1;
	ss<<" to ";
	ss<<end2;

//	printmsg(ss.str(),0);

	if(end1 <= end2)
	{
		if( end1<= ackNo && ackNo<= end2)
		{

			return 1;
		}
		else
		{

			return 0;
		}
	}
	else
	{
		if( (0<=ackNo && ackNo<=end2) || (end1<=ackNo && ackNo<= (availableSeqNos -1) ) )
		{

				return 1;
			}
		else
		{
			{

					return 0;
				}
		}

	}

}

void moveSndrWindow()
{

//	printmsg("inside moveSndrvWindow",0,0);
	int currentPtr  = 0;

	while ( pendingCount > 0 && nextSeqNumber != (sendBase +  WINDOWSIZE) % availableSeqNos )
	{
		currentPtr = sndBuffer.size()-pendingCount;
		stringstream ss("");
		//ss<< "Packet "<<buffer[currentPtr].seqnum<<" sent from A to layer 3";

		sndBuffer[currentPtr].datapkt.seqnum = nextSeqNumber;
		sndBuffer[currentPtr].datapkt.checksum = getCheckSum(&sndBuffer[currentPtr].datapkt);
		sndBuffer[currentPtr].transmissionTime = time_local;

		ss<< "A -> LAYER_3 : seqno: "<<sndBuffer[currentPtr].datapkt.seqnum;
		printmsg(ss.str(),0,0);

		//send the packet to layer 3
		tolayer3(0,sndBuffer[currentPtr].datapkt);

		float currentTime = time_local;
		//insert entry into timer queue.
		if(timerQueue.empty())
		{
			//start the timer and insert the record into queue
			starttimer(0,getTimeOut());
		}

		//insert the element in timer queue
		struct queueElem elem = {currentTime+getTimeOut(), sndBuffer[currentPtr].datapkt.seqnum,globleInsertionSeq};
		timerQueue.push(elem);
		globleInsertionSeq++;
		ss_msg.str(string());
		ss_msg<<"A: setting expected timeout for packet ";
		ss_msg<<elem.seqNo;
		ss_msg<<" to ";
		ss_msg<<elem.timeOut;
		printmsg(ss_msg.str(),0,0);

		//increment the counter pktCount_snd_transport_A
		pktCount_snd_transport_A++;

		//decrement the pending count
		pendingCount--;

//		if(sendBase == nextSeqNumber)
//		{
//		 starttimer(0,timeoutVal);
//		}

		//increment the nextSeqNo
		nextSeqNumber = (nextSeqNumber+1) % availableSeqNos;
	}

//	printmsg("Exiting moveSndrvWindow",0,0);
}

void moveRecvWindow()
{
	//printmsg("inside moveRecvWindow",0,1);
	while(recvBuffer[recvBaseBuffPos].isRecved == true && recvBuffer[recvBaseBuffPos].isDelivered == false)
		{
		pktCount_rcv_appln_B++;
		recvBuffer[recvBaseBuffPos].isDelivered = true;
		tolayer5(1,recvBuffer[recvBaseBuffPos].datapkt.payload);
		ss_msg.str(string());
		ss_msg<<"B -> LAYER_5 : seqno: ";
		ss_msg<<recvBuffer[recvBaseBuffPos].datapkt.seqnum;
		ss_msg<<"delivered. Messagecount: " ;
		ss_msg<<pktCount_rcv_appln_B;
		printmsg(ss_msg.str(),0,1);
		recvBase = (recvBase + 1)%availableSeqNos;
		recvBaseBuffPos= (recvBaseBuffPos + 1)% (sizeof recvBuffer / sizeof (pktEncap));
		}

	//printmsg("Exiting moveRecvWindow",0,1);
}



/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/


/* called from layer 5, passed the data to be sent to other side */
void A_output(struct msg message) //ram's comment - students can change the return type of the function from struct to pointers if necessary
{

		// make new packet
		struct pktEncap newPkt;
		memcpy(&newPkt.datapkt.payload,&message,sizeof newPkt.datapkt.payload);
		newPkt.isAcked  = false;
		newPkt.datapkt.acknum = 0;
		newPkt.isRetransmitted = false;

		// add the packet to buffer
		addToSndBuffer(newPkt);
		pendingCount++;

		moveSndrWindow();

}

void B_output(struct msg message)  /* need be completed only for extra credit */
// ram's comment - students can change the return type of this function from struct to pointers if needed  
{

}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet)
{
		//printmsg("A_input called",0);
		int corrupt = 1;

		// validate the checksum
		if(packet.checksum == getCheckSum(&packet))
		corrupt = 0;

		if(corrupt)
		{
			printmsg("LAYER_3 -> A : Corrupt packet received.",0,0);
			// do nothing
		}
		else
		if(isOutofWindow(packet.acknum,sendBase,nextSeqNumber))
		{
					printmsg("LAYER_3 -> A: Duplicate acknowledgment",0,0);
		}
		else
		{
//			//if received ack is for packet that is going to get expire earliest then stop the timer.
//			if (packet.acknum == timerQueue.top().seqNo )
//				stoptimer(0);

			//TODO:Delete below check once everything is running fine.
			if( !sndBuffer.empty() && sndBuffer.front().datapkt.seqnum != sendBase )
			{
				printmsg("Error in synchronizing buffer with base",1,0);
				stringstream ss1;
				ss1<<"Buffer.front.seqnum = ";
				ss1<<sndBuffer.front().datapkt.seqnum;
				ss1<<" Sendbase : ";
				ss1<<sendBase;
				printmsg(ss1.str(),1,0);
			}

			 //mark the packet as acknowledged
			int buffLoc = (packet.acknum+availableSeqNos - sendBase) % availableSeqNos;
			sndBuffer[buffLoc].isAcked = true;
			if(!sndBuffer[buffLoc].isRetransmitted)
			{
				ss_msg.str(string());

				estimatedRTT = (1- alpha) * estimatedRTT + alpha* (time_local - sndBuffer[buffLoc].transmissionTime);
				devRTT = (1-beta) * devRTT + beta* abs((time_local - sndBuffer[buffLoc].transmissionTime) - estimatedRTT);
				estimatedTimeout = estimatedRTT  +  devMultFact * devRTT;
				ss_msg<<"RTT :"<< (time_local - sndBuffer[buffLoc].transmissionTime)<< " EstimatedTimeout = "<<estimatedTimeout;
				printmsg(ss_msg.str(),0,0);
			}


			ss_msg.str(string());
			ss_msg<<"LAYER_3 -> A : ackno :";
			ss_msg<<packet.acknum;
			printmsg(ss_msg.str(),0,0);

			// move the window to the first unacked packet.
			while( !sndBuffer.empty() && sndBuffer[0].isAcked == 1 )
				{
					ss_msg.str(string());
					ss_msg<<"Current buffer front :"<<sndBuffer.begin()->datapkt.seqnum <<", CurrentSendbase: "<<sendBase <<" Current nextSeqNo "<<nextSeqNumber <<endl;
					sndBuffer.erase(sndBuffer.begin());
					sendBase = (sendBase + 1 )%availableSeqNos; // can put a check if it exceeds next seq No
					bufferCapacity_sndr++;
					ss_msg<<"New buffer front :"<<sndBuffer.begin()->datapkt.seqnum <<", new Sendbase: "<<sendBase <<endl;
				//	printmsg(ss_msg.str(),0,0);
				}

			ss_msg.str(string());
			ss_msg<<"A: New sendBase :";
			ss_msg<<sendBase;
			//printmsg(ss_msg.str(),0,0);

			moveSndrWindow();

		}

}

/* called when A's timer goes off */
void A_timerinterrupt() //ram's comment - changed the return type to void.
{
	//printmsg("inside A_timerinterrupt",0,0);
	int currentPosInBuff = (timerQueue.top().seqNo + availableSeqNos - sendBase)% availableSeqNos;
	if( !isOutofWindow(timerQueue.top().seqNo ,sendBase,nextSeqNumber)
		&& sndBuffer[currentPosInBuff].isAcked  == 0 )
	{

		// current top packet is unacknowledged.Need to resend it and set newtimeout for it.
		ss_msg.str(string());
		ss_msg<< sndBuffer[currentPosInBuff].datapkt.seqnum <<" at "<<time_local;

		printmsg("A: Timeout for packet "+ ss_msg.str(),0,0);
		printmsg("A -> LAYER_3 : seqno: "+ ss_msg.str(),0,0);

		sndBuffer[currentPosInBuff].isRetransmitted = true;
		tolayer3(0,sndBuffer[currentPosInBuff].datapkt);
		pktCount_snd_transport_A++;

		struct queueElem newelem = timerQueue.top();
		timerQueue.pop();
		newelem.timeOut = time_local  + getTimeOut();
		newelem.insertionSeq = globleInsertionSeq;
		timerQueue.push(newelem);
		globleInsertionSeq++;
		ss_msg.str(string());
		ss_msg<<"setting expected timeout for packet ";
		ss_msg<<newelem.seqNo;
		ss_msg<<" to ";
		ss_msg<<newelem.timeOut;
		//printmsg(ss_msg.str(),0,0);
	}

	//remove all the acked packets from the front of the queue
	while( !timerQueue.empty() && ( isOutofWindow(timerQueue.top().seqNo ,sendBase,nextSeqNumber)
		   || sndBuffer[( timerQueue.top().seqNo + availableSeqNos - sendBase)% availableSeqNos].isAcked  == 1 ))
	{
		//current top packet already acked  or out of window. Just pop it from the queue.
		timerQueue.pop();
	}

	if(!timerQueue.empty())
	 {
		float newTimeOutVal = timerQueue.top().timeOut - time_local ;
		//start the timer
		starttimer(0,newTimeOutVal);
	 }

	//printmsg("exiting A_timerinterrupt",0,0);

}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init() //ram's comment - changed the return type to void.
{

	//set current buffer capacity
	bufferCapacity_sndr = BUFFERSIZE;

	// set the base and next seq no equal to the agreed value from TCP connection setup
	sendBase  = initSeqNo;
	nextSeqNumber  = initSeqNo;

	pendingCount= 0;
	//printmsg("A_init complete",0);

	ss_msg.str(string());

	estimatedRTT = timeoutVal;
	devRTT = initRTTDev;
	estimatedTimeout = estimatedRTT  +  devMultFact * devRTT;

}


/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet)
{
	//increment the counter pktCount_rcv_transport_B
	pktCount_rcv_transport_B ++;

	int valid = 0;
	int corrupt = 1;
	//validate the packet
	if(packet.checksum == getCheckSum(&packet))
		corrupt = 0;

	// validate the sequence number
	if( ! isOutofWindow(packet.seqnum , recvBase,(recvBase + WINDOWSIZE-1) % availableSeqNos))
		valid = 1;

		stringstream ss;
		ss<<packet.seqnum;
		string seqNo(ss.str());

		if(corrupt) // corrupt packet. Resend last ack packet
		 {
			 printmsg("LAYER_3 -> B: Corrupt packet.",0,1); // do we send ack for corrupt packets in SR ?
			 //tolayer3(1,recvpkt); // TODO : Make sure ack packet is properly initialized
		 }
		 else if (!valid) // invalid ack. Resend last ack packet
		 {

			printmsg("LAYER_3 -> B: Out of window packet(seqno:"+ seqNo +") received. Resending the ack.",0,1);

			// make and send ack packet
			 recvpkt.acknum = packet.seqnum;
			 recvpkt.checksum = getCheckSum(&recvpkt);

			 printmsg("B -> LAYER_3: ackno "+seqNo,0,1);
			// no need to count
			 tolayer3(1,recvpkt);

		 }
		 else // valid packet.
		 {
			 struct pktEncap newpkt;
			 newpkt.datapkt = packet;
			 newpkt.isRecved = true;
			 newpkt.isDelivered = false;

			 //buffer the packet.
			 addToRecvBuffer(newpkt);

			 // print info message
			 printmsg("LAYER_3 -> B : seqno "+seqNo+" received at B.",0,1);

			 // make and send ack packet
			 recvpkt.acknum = packet.seqnum;
			 recvpkt.checksum = getCheckSum(&recvpkt);

			 printmsg("B -> LAYER_3: ackno "+seqNo,0,1);
			 // no need to count
			 tolayer3(1,recvpkt);

			 // deliver the in-order packets if any
			 moveRecvWindow();

		 }

}

/* called when B's timer goes off */
void B_timerinterrupt() //ram's comment - changed the return type to void.
{
}

/* the following routine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init() //ram's comment - changed the return type to void.
{
	// set initial seqNo that is set in 3 - way handshake
	recvBase = initSeqNo;

	memset(&recvpkt,0,sizeof recvpkt);
	// If first packet from  A gets corrupted, then B will send the last saved packet.
	// Packet should be with acknum = previous seqno to the initseqno
	recvpkt.acknum =  (initSeqNo + availableSeqNos -1) % availableSeqNos ;

	// not used. Just initializing to zero
	recvpkt.seqnum = 0;

	//set the checksum
	recvpkt.checksum = getCheckSum(&recvpkt);

	bufferCapacity_recv = BUFFERSIZE;
	recvBaseBuffPos = 0;

	memset(recvBuffer,0,sizeof recvBuffer);


	//printmsg("A_init complete",0);
}

int TRACE = 1;             /* for my debugging */
int nsim = 0;              /* number of messages from 5 to 4 so far */
int nsimmax = 0;           /* number of msgs to generate, then stop */
float time_local = 0;
float lossprob;            /* probability that a packet is dropped  */
float corruptprob;         /* probability that one bit is packet is flipped */
float lambda;              /* arrival rate of messages from layer 5 */
int   ntolayer3;           /* number sent into layer 3 */
int   nlost;               /* number lost in media */
int ncorrupt;              /* number corrupted by media*/

/****************************************************************************/
/* jimsrand(): return a float in range [0,1].  The routine below is used to */
/* isolate all random number generation in one location.  We assume that the*/
/* system-supplied rand() function return an int in therange [0,mmm]        */
/****************************************************************************/
float jimsrand() 
{
  double mmm = 2147483647;   /* largest int  - MACHINE DEPENDENT!!!!!!!!   */
  float x;                   /* individual students may need to change mmm */ 
  x = rand()/mmm;            /* x should be uniform in [0,1] */
  return(x);
}  


/*****************************************************************
***************** NETWORK EMULATION CODE IS BELOW ***********
The code below emulates the layer 3 and below network environment:
  - emulates the tranmission and delivery (possibly with bit-level corruption
    and packet loss) of packets across the layer 3/4 interface
  - handles the starting/stopping of a timer, and generates timer
    interrupts (resulting in calling students timer handler).
  - generates message to be sent (passed from later 5 to 4)

THERE IS NOT REASON THAT ANY STUDENT SHOULD HAVE TO READ OR UNDERSTAND
THE CODE BELOW.  YOU SHOLD NOT TOUCH, OR REFERENCE (in your code) ANY
OF THE DATA STRUCTURES BELOW.  If you're interested in how I designed
the emulator, you're welcome to look at the code - but again, you should have
to, and you defeinitely should not have to modify
******************************************************************/



/* possible events: */
#define  TIMER_INTERRUPT 0  
#define  FROM_LAYER5     1
#define  FROM_LAYER3     2

#define  OFF             0
#define  ON              1
#define   A    0
#define   B    1


struct event {
   float evtime;           /* event time */
   int evtype;             /* event type code */
   int eventity;           /* entity where event occurs */
   struct pkt *pktptr;     /* ptr to packet (if any) assoc w/ this event */
   struct event *prev;
   struct event *next;
 };
struct event *evlist = NULL;   /* the event list */


void insertevent(struct event *p)
{
   struct event *q,*qold;

   if (TRACE>2) {
      printf("            INSERTEVENT: time is %lf\n",time_local);
      printf("            INSERTEVENT: future time will be %lf\n",p->evtime); 
      }
   q = evlist;     /* q points to header of list in which p struct inserted */
   if (q==NULL) {   /* list is empty */
        evlist=p;
        p->next=NULL;
        p->prev=NULL;
        }
     else {
        for (qold = q; q !=NULL && p->evtime > q->evtime; q=q->next)
              qold=q; 
        if (q==NULL) {   /* end of list */
             qold->next = p;
             p->prev = qold;
             p->next = NULL;
             }
           else if (q==evlist) { /* front of list */
             p->next=evlist;
             p->prev=NULL;
             p->next->prev=p;
             evlist = p;
             }
           else {     /* middle of list */
             p->next=q;
             p->prev=q->prev;
             q->prev->next=p;
             q->prev=p;
             }
         }
}





/********************* EVENT HANDLINE ROUTINES *******/
/*  The next set of routines handle the event list   */
/*****************************************************/

void generate_next_arrival()
{
   double x,log(),ceil();
   struct event *evptr;
//    //char *malloc();
   float ttime;
   int tempint;

   if (TRACE>2)
       printf("          GENERATE NEXT ARRIVAL: creating new arrival\n");

   x = lambda*jimsrand()*2;  /* x is uniform on [0,2*lambda] */
                             /* having mean of lambda        */

   evptr = (struct event *)malloc(sizeof(struct event));
   evptr->evtime =  time_local + x;
   evptr->evtype =  FROM_LAYER5;
   if (BIDIRECTIONAL && (jimsrand()>0.5) )
      evptr->eventity = B;
    else
      evptr->eventity = A;
   insertevent(evptr);
}





void init()                         /* initialize the simulator */
{
  int i;
  float sum, avg;
  float jimsrand();
  
  
   printf("-----  Stop and Wait Network Simulator Version 1.1 -------- \n\n");
   printf("Enter the number of messages to simulate: ");
   scanf("%d",&nsimmax);
   printf("Enter  packet loss probability [enter 0.0 for no loss]:");
   scanf("%f",&lossprob);
   printf("Enter packet corruption probability [0.0 for no corruption]:");
   scanf("%f",&corruptprob);
   printf("Enter average time between messages from sender's layer5 [ > 0.0]:");
   scanf("%f",&lambda);
   printf("Enter TRACE:");
   scanf("%d",&TRACE);

   srand(9999);              /* init random number generator */
   sum = 0.0;                /* test random number generator for students */
   for (i=0; i<1000; i++)
      sum=sum+jimsrand();    /* jimsrand() should be uniform in [0,1] */
   avg = sum/1000.0;
   if (avg < 0.25 || avg > 0.75) {
    printf("It is likely that random number generation on your machine\n" ); 
    printf("is different from what this emulator expects.  Please take\n");
    printf("a look at the routine jimsrand() in the emulator code. Sorry. \n");
    exit(0);
    }

   ntolayer3 = 0;
   nlost = 0;
   ncorrupt = 0;

   time_local=0;                    /* initialize time to 0.0 */
   generate_next_arrival();     /* initialize event list */
}






//int TRACE = 1;             /* for my debugging */
//int nsim = 0;              /* number of messages from 5 to 4 so far */ 
//int nsimmax = 0;           /* number of msgs to generate, then stop */
//float time = 0.000;
//float lossprob;            /* probability that a packet is dropped  */
//float corruptprob;         /* probability that one bit is packet is flipped */
//float lambda;              /* arrival rate of messages from layer 5 */   
//int   ntolayer3;           /* number sent into layer 3 */
//int   nlost;               /* number lost in media */
//int ncorrupt;              /* number corrupted by media*/

int main()
{
   struct event *eventptr;
   struct msg  msg2give;
   struct pkt  pkt2give;
   
   int i,j;
   char c; 
  
   init();
   A_init();
   B_init();
   

   while (1) {
        eventptr = evlist;            /* get next event to simulate */
        if (eventptr==NULL)
           goto terminate;
        evlist = evlist->next;        /* remove this event from event list */
        if (evlist!=NULL)
           evlist->prev=NULL;
        if (TRACE>=2) {
           printf("\nEVENT time: %f,",eventptr->evtime);
           printf("  type: %d",eventptr->evtype);
           if (eventptr->evtype==0)
	       printf(", timerinterrupt  ");
             else if (eventptr->evtype==1)
               printf(", fromlayer5 ");
             else
	     printf(", fromlayer3 ");
           printf(" entity: %d\n",eventptr->eventity);
           }
        time_local = eventptr->evtime;        /* update time to next event time */
        if (nsim==nsimmax)
	  break;                        /* all done with simulation */
        if (eventptr->evtype == FROM_LAYER5 ) {
            generate_next_arrival();   /* set up future arrival */
            /* fill in msg to give with string of same letter */    
            j = nsim % 26; 
            for (i=0; i<20; i++)  
               msg2give.data[i] = 97 + j;
            if (TRACE>2) {
               printf("          MAINLOOP: data given to student: ");
                 for (i=0; i<20; i++) 
                  printf("%c", msg2give.data[i]);
               printf("\n");
	     }
            nsim++;
            if (eventptr->eventity == A) 
               A_output(msg2give);  
             else
               B_output(msg2give);  
            }
          else if (eventptr->evtype ==  FROM_LAYER3) {
            pkt2give.seqnum = eventptr->pktptr->seqnum;
            pkt2give.acknum = eventptr->pktptr->acknum;
            pkt2give.checksum = eventptr->pktptr->checksum;
            for (i=0; i<20; i++)  
                pkt2give.payload[i] = eventptr->pktptr->payload[i];
	    if (eventptr->eventity ==A)      /* deliver packet by calling */
   	       A_input(pkt2give);            /* appropriate entity */
            else
   	       B_input(pkt2give);
	    free(eventptr->pktptr);          /* free the memory for packet */
            }
          else if (eventptr->evtype ==  TIMER_INTERRUPT) {
            if (eventptr->eventity == A) 
	       A_timerinterrupt();
             else
	       B_timerinterrupt();
             }
          else  {
	     printf("INTERNAL PANIC: unknown event type \n");
             }
        free(eventptr);
        }

terminate:
   printf(" Simulator terminated at time %f\n after sending %d msgs from layer5\n",time_local,nsim);
   printResult(time_local,nsim);
 //  storeValue(time_local,nsim);
}


/********************* EVENT HANDLINE ROUTINES *******/
/*  The next set of routines handle the event list   */
/*****************************************************/
 
/*void generate_next_arrival()
{
   double x,log(),ceil();
   struct event *evptr;
    //char *malloc();
   float ttime;
   int tempint;

   if (TRACE>2)
       printf("          GENERATE NEXT ARRIVAL: creating new arrival\n");
 
   x = lambda*jimsrand()*2;  // x is uniform on [0,2*lambda] 
                             // having mean of lambda       
   evptr = (struct event *)malloc(sizeof(struct event));
   evptr->evtime =  time + x;
   evptr->evtype =  FROM_LAYER5;
   if (BIDIRECTIONAL && (jimsrand()>0.5) )
      evptr->eventity = B;
    else
      evptr->eventity = A;
   insertevent(evptr);
} */




void printevlist()
{
  struct event *q;
  int i;
  printf("--------------\nEvent List Follows:\n");
  for(q = evlist; q!=NULL; q=q->next) {
    printf("Event time: %f, type: %d entity: %d\n",q->evtime,q->evtype,q->eventity);
    }
  printf("--------------\n");
}



/********************** Student-callable ROUTINES ***********************/

/* called by students routine to cancel a previously-started timer */
void stoptimer(int AorB)
 //AorB;  /* A or B is trying to stop timer */
{
 struct event *q,*qold;

 if (TRACE>2)
    printf("          STOP TIMER: stopping timer at %f\n",time_local);
/* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next)  */
 for (q=evlist; q!=NULL ; q = q->next) 
    if ( (q->evtype==TIMER_INTERRUPT  && q->eventity==AorB) ) { 
       /* remove this event */
       if (q->next==NULL && q->prev==NULL)
             evlist=NULL;         /* remove first and only event on list */
          else if (q->next==NULL) /* end of list - there is one in front */
             q->prev->next = NULL;
          else if (q==evlist) { /* front of list - there must be event after */
             q->next->prev=NULL;
             evlist = q->next;
             }
           else {     /* middle of list */
             q->next->prev = q->prev;
             q->prev->next =  q->next;
             }
       free(q);
       return;
     }
  printf("Warning: unable to cancel your timer. It wasn't running.\n");
}


void starttimer(int AorB,float increment)
// AorB;  /* A or B is trying to stop timer */

{

 struct event *q;
 struct event *evptr;
 ////char *malloc();

 if (TRACE>2)
    printf("          START TIMER: starting timer at %f\n",time_local);
 /* be nice: check to see if timer is already started, if so, then  warn */
/* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next)  */
   for (q=evlist; q!=NULL ; q = q->next)  
    if ( (q->evtype==TIMER_INTERRUPT  && q->eventity==AorB) ) { 
      printf("Warning: attempt to start a timer that is already started\n");
      return;
      }
 
/* create future event for when timer goes off */
   evptr = (struct event *)malloc(sizeof(struct event));
   evptr->evtime =  time_local + increment;
   evptr->evtype =  TIMER_INTERRUPT;
   evptr->eventity = AorB;
   insertevent(evptr);
} 


/************************** TOLAYER3 ***************/
void tolayer3(int AorB,struct pkt packet)
{
 struct pkt *mypktptr;
 struct event *evptr,*q;
 ////char *malloc();
 float lastime, x, jimsrand();
 int i;


 ntolayer3++;

 /* simulate losses: */
 if (jimsrand() < lossprob)  {
      nlost++;
      if (TRACE>0)    
	printf("          TOLAYER3: packet being lost\n");
      return;
    }  

/* make a copy of the packet student just gave me since he/she may decide */
/* to do something with the packet after we return back to him/her */ 
 mypktptr = (struct pkt *)malloc(sizeof(struct pkt));
 mypktptr->seqnum = packet.seqnum;
 mypktptr->acknum = packet.acknum;
 mypktptr->checksum = packet.checksum;
 for (i=0; i<20; i++)
    mypktptr->payload[i] = packet.payload[i];
 if (TRACE>2)  {
   printf("          TOLAYER3: seq: %d, ack %d, check: %d ", mypktptr->seqnum,
	  mypktptr->acknum,  mypktptr->checksum);
    for (i=0; i<20; i++)
        printf("%c",mypktptr->payload[i]);
    printf("\n");
   }

/* create future event for arrival of packet at the other side */
  evptr = (struct event *)malloc(sizeof(struct event));
  evptr->evtype =  FROM_LAYER3;   /* packet will pop out from layer3 */
  evptr->eventity = (AorB+1) % 2; /* event occurs at other entity */
  evptr->pktptr = mypktptr;       /* save ptr to my copy of packet */
/* finally, compute the arrival time of packet at the other end.
   medium can not reorder, so make sure packet arrives between 1 and 10
   time units after the latest arrival time of packets
   currently in the medium on their way to the destination */
 lastime = time_local;
/* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next) */
 for (q=evlist; q!=NULL ; q = q->next) 
    if ( (q->evtype==FROM_LAYER3  && q->eventity==evptr->eventity) ) 
      lastime = q->evtime;
 evptr->evtime =  lastime + 1 + 9*jimsrand();
 


 /* simulate corruption: */
 if (jimsrand() < corruptprob)  {
    ncorrupt++;
    if ( (x = jimsrand()) < .75)
       mypktptr->payload[0]='Z';   /* corrupt payload */
      else if (x < .875)
       mypktptr->seqnum = 999999;
      else
       mypktptr->acknum = 999999;
    if (TRACE>0)    
	printf("          TOLAYER3: packet being corrupted\n");
    }  

  if (TRACE>2)  
     printf("          TOLAYER3: scheduling arrival on other side\n");
  insertevent(evptr);
} 

void tolayer5(int AorB,char *datasent)
{
  
  int i;  
  if (TRACE>2) {
     printf("          TOLAYER5: data received: ");
     for (i=0; i<20; i++)  
        printf("%c",datasent[i]);
     printf("\n");
   }
  
}
