#ifndef CONST_H
#define CONST_H


#define _RECV_WAIT 10000
#define _MSG_WAIT 200

#define NUMSIZE 4		// size of int in bytes
#define BUFLEN 512
#define FILENMLEN 32
#define NO_OF_TRIES 5

#define OPCODELEN 3		// in bits
#define BATCHNOLEN 5	// in bits
#define MAXBATCHNUM 0x1F

#define BATCHESPOSSB 32	// no of batches that can handle
#define PARTSINBATCH 4	// no of parts in batch
#define PARTSIZE 8 // bytes in part
#define BATCHSIZE PARTSIZE * PARTSINBATCH
#define BATCH_CYCLE_SIZE BATCHSIZE * BATCHESPOSSB


#define P1 5432
#define P2 5431

#endif
