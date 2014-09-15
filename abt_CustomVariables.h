/*This header contains all the constants required by Alternating bit protocol. */
#ifndef ABT_CUSTOMVARIABLES_H
#define ABT_CUSTOMVARIABLES_H
enum senderState{DATA_ZERO_SNDR,DATA_ONE_SNDR,ACK_ZERO_SNDR,ACK_ONE_SNDR};
enum receiverState{DATA_ZERO_RECV,DATA_ONE_RECV};
float const timeoutVal = 15.0;
int enableMsgLog = 1;

//not required for abt. Will be ignored
int const WINDOWSIZE  =10;

#endif
