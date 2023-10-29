#ifndef OPS_H
#define OPS_H


#define OPidx 0		// index of operation byte in message buffer
#define FLidx 1		// index of starting of filename in message buffer
#define PRTidx 0 	// index of partno in message buffer
#define BTCidx 1 	// index of batchno in message buffer
#define DATAidx 5	// index of data bytes 


// operation codes
#define SENDING 0x01
#define SENDBATCH 0x02
#define RECEIVED 0x03
#define RESENDPARTS 0x04

#define DATA 0x80
#endif
