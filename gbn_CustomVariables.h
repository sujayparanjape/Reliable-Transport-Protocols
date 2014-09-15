/*This header contains all the constants required by Go-Back-N protocol. */
#ifndef GBN_CUSTOMVARIABLES_H
#define GBN_CUSTOMVARIABLES_H

/*timeout value */
float const timeoutVal = 15.0;

/*variable to enable or disable printing of event messages.
 * 1 = Enable , 0= Disable
 */
int enableMsgLog = 1;

// Buffeer size
int const BUFFERSIZE = 1000;

// Window size
int const WINDOWSIZE  =10;

// init seq no
int initSeqNo= 0 ;

//Available seq number count. Can be anything greater than windowsize for GBN
int const availableSeqNos= WINDOWSIZE + 1 ;

#endif
