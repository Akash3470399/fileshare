#include "intr/const.h"
#include "intr/part.h"

#ifndef RECEIVER_H
#define RECEIVER_H


extern unsigned char sbuf[BUFLEN];
extern unsigned int smsglen;

extern unsigned char rbuf[BUFLEN];
extern unsigned int rmsglen;


typedef struct rfileinfo rfileinfo;
struct rfileinfo
{
	char name[FILENMLEN];
	unsigned int size;
	struct prtdata *pd;
};



void receive_file(char *filename);
int resp_handshake(rfileinfo *finfo);
int reqst_batch(rfileinfo *finfo, unsigned int batchno);
int writetofile(FILE *recvfp, unsigned int batchno);
int receive_batchwise(rfileinfo *finfo);
int receive_batch(rfileinfo *finfo, unsigned int batches_recv);
int recover_parts(rfileinfo *finfo, unsigned int batches_recv);
#endif
