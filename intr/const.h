#ifndef CONST_H
#define CONST_H


#define _RECV_WAIT 10000
#define _MSG_WAIT 5 

#define NUMSIZE 4		// size of int in bytes
#define BUFLEN 520 
#define FILENMLEN 32
#define NO_OF_TRIES 20

#define OPCODELEN 3		// in bits
#define BATCHNOLEN 5	// in bits

#define PARTSINBATCH 64	// no of parts in batch
#define PARTSIZE 1 // bytes in part
#define BATCHSIZE PARTSIZE * PARTSINBATCH

#define P1 5431
#define P2 5432

#endif
