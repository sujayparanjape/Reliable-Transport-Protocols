#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <cstring>
#include "abt_CustomVariables.h"
#include <sstream>
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

/******** STATIC VARIABLES *********/

enum senderState state_sndr;
enum receiverState state_recv;
struct pkt sndpkt,recvpkt;
float timout_a;// = 20.0;
string const protocolName = "Alternating-bit";
int pktCount_snd_transport_A = 0;
int pktCount_rcv_transport_B = 0;
int pktCount_rcv_appln_B = 0 ;

stringstream ss_msg;

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

/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/

/* called from layer 5, passed the data to be sent to other side */
void A_output(struct msg message) //ram's comment - students can change the return type of the function from struct to pointers if necessary
{
	switch (state_sndr)
	{
	case DATA_ZERO_SNDR:
		{
			//make packet
			struct pkt newPkt;
			memcpy(&newPkt.payload,&message,sizeof &newPkt.payload);
			newPkt.seqnum = 0;
			newPkt.acknum = 0;
			newPkt.checksum = getCheckSum(&newPkt);

			//change the static copy
			sndpkt = newPkt;

			//print message
			printmsg("A -> LAYER_3 : seq 0",0,0);

			//send packet
			tolayer3(0,newPkt);

			//increment the counter pktCount_snd_transport_A
			pktCount_snd_transport_A++;

			//start the timer
			starttimer(0,timout_a);

			//change state to ACK_ZERO_SNDR
			state_sndr = ACK_ZERO_SNDR;


		}
		break;
	case DATA_ONE_SNDR:
		{
			//make packet
			struct pkt newPkt;
			memcpy(&newPkt.payload,&message,sizeof &newPkt.payload);
			newPkt.seqnum = 1;
			newPkt.acknum = 0;
			newPkt.checksum = getCheckSum(&newPkt);

			//change the static copy
			sndpkt = newPkt;

			//print message
			printmsg("A -> LAYER_3 : seq 1",0,0);

			//send packet
			tolayer3(0,newPkt);

			//increment the counter pktCount_snd_transport_A
			pktCount_snd_transport_A++;

			//start the timer
			starttimer(0,timout_a);

			//change state to ACK_ZERO_SNDR
			state_sndr = ACK_ONE_SNDR;


			//printmsg("Packet 1 sent from A to layer 3",0);
		}
		break;

	case ACK_ZERO_SNDR:
		{
			// ignore packet
			printmsg("A: A_output called when in waitForACK0 state. Ignoring the packet.",0,0);
		}
		break;
	case ACK_ONE_SNDR:
		{
			// ignore packet
			printmsg("A: A_output called when in waitForACK1 state. Ignoring the packet.",0,0);
		}
		break;
	}

}

void B_output(struct msg message)  /* need be completed only for extra credit */
// ram's comment - students can change the return type of this function from struct to pointers if needed  
{

}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet)
{
 switch(state_sndr)
 	 {
 	 case DATA_ZERO_SNDR:
 		 printmsg("LAYER_3 -> A: A_input called when in waitfor_0_fromabove state. Ignoring the packet.",0,0);
 		 break;
 	 case DATA_ONE_SNDR:
 		 printmsg("LAYER_3 -> A: A_input called when in waitfor_1_fromabove state. Ignoring the packet.",0,0);
 		 break;
 	 case ACK_ZERO_SNDR:
 	 	 {
 	 		 int corrupt = 1;
 	 		 int valid = 0;
 	 		 int expectedAck = 0;

 	 		 // validate the checksum
 	 		 if(packet.checksum == getCheckSum(&packet))
 	 			 corrupt = 0;

 	 		 if(!corrupt) // valid packet
 	 		 {
 	 			 //validate the expectes seq no
 	 			 if(packet.acknum == expectedAck)
 	 			 valid=1;
 	 		 }

 	 		 if(corrupt) // corrupt packet. Resend last packet
 	 		 {
 	 			printmsg("LAYER_3 -> A: Corrupt packet received at A. No action taken.",0,0);
// 	 			 printmsg("Corrupt packet received at A. Resending the last packet.",0);
// 	 			 tolayer3(0,sndpkt);
//
// 	 			//increment the counter pktCount_snd_transport_A
// 	 			pktCount_snd_transport_A++;
//
// 	 			 //restart the timer
// 	 			 starttimer(0,timout_a);
 	 		 }
 	 		 else if (!valid) // invalid ack. resend last packet
 	 		 {
 	 			printmsg("LAYER_3 -> A: Invalid ack number(1) received. No action taken.",0,0);
// 	 			printmsg("Invalid ack number(1) received. Resending the last packet.",0);
// 	 			tolayer3(0,sndpkt);
//
// 	 			//increment the counter pktCount_snd_transport_A
// 	 			pktCount_snd_transport_A++;
//
// 	 			//restart the timer
// 	 			starttimer(0,timout_a);
 	 		 }
 	 		 else // valid packet. change state to waitfor1fromabove.
 	 		 {
 	 	 		 //stop timer
 	 	 	 	 stoptimer(0);
 	 			 printmsg("LAYER_3 -> A: Ack0 received. Changing the state to waitFor1FromAbove.",0,0);
 	 			 state_sndr = DATA_ONE_SNDR;
 	 		 }
 	 	 }
 		 break;

 	 case ACK_ONE_SNDR:
		{

			 int corrupt = 1;
			 int valid = 0;
			 int expectedAck = 1;

			 // validate the checksum
			 if(packet.checksum == getCheckSum(&packet))
				 corrupt = 0;

			 if(!corrupt) // valid packet
			 {
				 //validate the expected seq no
				 if(packet.acknum == expectedAck)
				 valid=1;
			 }

			 if(corrupt) // corrupt packet. Resend last packet
			 {
				 printmsg("LAYER_3 -> A: Corrupt packet received at A. No action taken.",0,0);
				 //printmsg("Corrupt packet received at A. Resending the last packet.",0);
//				 tolayer3(0,sndpkt);
//
//				 //increment the counter pktCount_snd_transport_A
//				 pktCount_snd_transport_A++;
//
//				 //restart the timer
//				 starttimer(0,timout_a);
			 }
			 else if (!valid) // invalid ack. resend last packet
			 {
				 printmsg("LAYER_3 -> A: Invalid ack number(0) received. No action taken.",0,0);
//				printmsg("Invalid ack number(0) received. Resending the last packet.",0);
//				tolayer3(0,sndpkt);
//
//				//increment the counter pktCount_snd_transport_A
//				pktCount_snd_transport_A++;
//
//				//restart the timer
//				starttimer(0,timout_a);
			 }
			 else // valid packet. change state to waitfor1fromabove.
			 {
				 //stop timer
	 			 stoptimer(0);

				 printmsg("LAYER_3 -> A: Ack1 received. Changing the state to waitFor0FromAbove.",0,0);
				 state_sndr = DATA_ZERO_SNDR;

			 }
		 }
 		 break;
 	 }
}

/* called when A's timer goes off */
void A_timerinterrupt() //ram's comment - changed the return type to void.
{
	//stoptimer(0); not required. Its already stopped when it enters this call.?

	ss_msg.str(string());
	ss_msg<<sndpkt.seqnum;
	printmsg("A: Timeout occurred. Re-sending the last packet("+ss_msg.str()+").",0,0);

	//send last packet
	printmsg("A -> LAYER_3: seq"+ss_msg.str(),0,0);
	tolayer3(0,sndpkt);

	//increment the counter pktCount_snd_transport_A
	pktCount_snd_transport_A++;

	//start the timer --- is it required to increment the timer value?
	starttimer(0,timout_a);

}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init() //ram's comment - changed the return type to void.
{
 state_sndr = DATA_ZERO_SNDR;
 memset(&sndpkt,0,sizeof sndpkt);
 timout_a =timeoutVal;
}


/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet)
{
 //increment the counter pktCount_rcv_transport_B
 pktCount_rcv_transport_B ++;

 switch(state_recv)
 {
 	 case DATA_ZERO_RECV:
 	 	 {
 	 		int expectedSeq = 0;
 	 		int valid = 0;
 	 		int corrupt = 1;
 	 		//validate the packet
 	 		if(packet.checksum == getCheckSum(&packet))
 	 			corrupt = 0;

 	 		// validate the sequence number
 	 		if(packet.seqnum == expectedSeq)
 	 		valid = 1;

 	 		ss_msg.str(string());
 	 		ss_msg<<recvpkt.acknum;
 	 		if(corrupt) // corrupt packet. Resend last ack packet
			 {

				 printmsg("LAYER_3 -> B: Corrupt packet received at B. Resending the last ack("+ss_msg.str()+") packet.",0,1);
				 printmsg("B -> LAYER_3: ack"+ss_msg.str(),0,1);
				 tolayer3(1,recvpkt); // TODO : Make sure ack packet is properly initialized
			 }
			 else if (!valid) // invalid ack. Resend last ack packet
			 {
				printmsg("LAYER_3 -> B: Invalid seq number(1) received. Resending the last ack("+ss_msg.str()+") packet.",0,1);
				printmsg("B -> LAYER_3: ack"+ss_msg.str(),0,1);
				tolayer3(1,recvpkt);
			 }
			 else // valid packet. change state to waitFor1FromBelow
			 {
				 //deliver to layer 5
				 tolayer5(1,packet.payload);

				 // increment the counter pktCount_rcv_appln_B
				 pktCount_rcv_appln_B++;

				 // print info message
				 printmsg("LAYER_3 -> B :seqno 0. Delivered to layer 5.Changing the state to waitFor1FromBelow.",0,1);

				 // change the state to waitFor1FromBelow
				 state_recv = DATA_ONE_RECV;

				 //make and send the ack packet
				 recvpkt.acknum = 0;
				 recvpkt.checksum = getCheckSum(&recvpkt);

				 printmsg("B -> LAYER_3: ack0",0,1);
				 // not required to count.
				 tolayer3(1,recvpkt);
			 }
 	 	 }
 		 break;
 	 case DATA_ONE_RECV:
 	 	 {
 	 		int expectedSeq = 1;
			int valid = 0;
			int corrupt = 1;
			//validate the packet
			if(packet.checksum == getCheckSum(&packet))
				corrupt = 0;

			// validate the sequence number
			if(packet.seqnum == expectedSeq)
			valid = 1;

			ss_msg.str(string());
			ss_msg<<recvpkt.acknum;
			if(corrupt) // corrupt packet. Resend last ack packet
			 {
				 printmsg("LAYER_3 -> B : Corrupt packet received at B. Resending the last ack("+ss_msg.str()+") packet.",0,1);
				 printmsg("B -> LAYER_3: ack"+ss_msg.str(),0,1);
				 tolayer3(1,recvpkt); // TODO : Make sure ack packet is properly initialized
			 }
			 else if (!valid) // invalid ack. Resend last ack packet
			 {

				printmsg("LAYER_3 -> B : Invalid seq number(0) received. Resending the last ack("+ss_msg.str()+") packet.",0,1);
				printmsg("B -> LAYER_3: ack"+ss_msg.str(),0,1);
				tolayer3(1,recvpkt);
			 }
			 else // valid packet. change state to waitFor0FromBelow
			 {
				 //deliver to layer 5
				 tolayer5(1,packet.payload);

				//increment the counter pktCount_rcv_appln_B
				 pktCount_rcv_appln_B++;

				 // print info message
				 printmsg("LAYER_3 -> B :seqno 1. Delivered to layer 5.Changing the state to waitFor0FromBelow.",0,1);
				 //printmsg("B: Packet 1 received. Changing the state to waitFor0FromBelow.",0);

				 // change the state
				 state_recv = DATA_ZERO_RECV;

				 // make and send ack packet
				 recvpkt.acknum = 1;
				 recvpkt.checksum = getCheckSum(&recvpkt);

				 // no need to count
				 printmsg("B -> LAYER_3: ack1",0,1);
				 tolayer3(1,recvpkt);
			 }
 	 	 }
 	 	 break;
 }
}

/* called when B's timer goes off */
void B_timerinterrupt() //ram's comment - changed the return type to void.
{
	//printmsg("B's timeout. Need to re-send the packet.",0);
}

/* the following routine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init() //ram's comment - changed the return type to void.
{
	state_recv = DATA_ZERO_RECV;
	memset(&recvpkt,0,sizeof recvpkt);
	// If first packet from  A gets corrupted, then B will send the last saved packet.
	// That should be with acknum = 1
	recvpkt.acknum = 1;

	// not used. Just initializing to zero
	recvpkt.seqnum = 0;

	//set the checksum
	recvpkt.checksum = getCheckSum(&recvpkt);

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
   //storeValue(time_local,nsim);
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
