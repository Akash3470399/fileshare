#ifndef CONST_H
#define CONST_H


#define _RECV_WAIT 10000
#define _MSG_WAIT 25 

#define NUMSIZE 4		// size of int in bytes
#define BUFLEN 1040 
#define FILENMLEN 32
#define NO_OF_TRIES 20

#define OPCODELEN 3		// in bits
#define BATCHNOLEN 5	// in bits

#define PARTSINBATCH 120	// no of parts in batch
#define PARTSIZE 1024 // bytes in part
#define BATCHSIZE PARTSIZE * PARTSINBATCH

#define P1 5432
#define P2 5432

#endif
