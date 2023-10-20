#include "intr/consts.h"
#include "intr/part.h"
#ifndef RECEIVER_H
#define RECEIVER_H

typedef struct rfileinfo rfileinfo;

struct rfileinfo
{
	char name[FILENMLEN + FILENMLEN];
	unsigned int size;
	struct prtdata *pd;
};

void receive_file();
int build_rcontext(rfileinfo *finfo);
int reqst_firstbatch(rfileinfo *finfo);
int reqst_batch(rfileinfo *finfo, unsigned int batchno);
int writetofile(FILE *recvfp, unsigned int batchno);
int receive_batch(rfileinfo *finfo, unsigned int curbatch);
unsigned int create_missing_part_request(unsigned char *part_arr, unsigned int arrsize, unsigned int batchno);
int recover_parts(rfileinfo *finfo, unsigned int curbatch);

// internal methods
void _saveContext(rfileinfo *);	
#endif
