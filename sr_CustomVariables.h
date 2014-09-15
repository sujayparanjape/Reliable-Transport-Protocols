/*This header contains all the constants required by Selective Repeat protocol. */
#ifndef SR_CUSTOMVARIABLES_H
#define SR_CUSTOMVARIABLES_H

/*timeout value */
float const timeoutVal = 15.0;

/*variable to enable or disable printing of event messages.
 * 1 = Enable , 0= Disable
 */
int enableMsgLog = 1;

// Buffer size
int const BUFFERSIZE = 1000;

// Window size
int const WINDOWSIZE = 10;

// init seq no, that is supposed to be agreed during handshake. 
int initSeqNo= 0 ;

//Available seq number count. Can be anything greater than equal to 2*windowsize for SR
int const availableSeqNos= WINDOWSIZE * 2 ;

/* Following variables are used to make timeout value adaptive.*/
float const initRTTDev = 0;

int const isAdaptiveRTT = 0;

float devMultFact = 0.5;

float alpha = 0.125;

float beta = 0.25;
/***********************************************************/

#endif
