#include "intr/const.h"
#include "intr/part.h"

#ifndef RECEIVER_H
#define RECEIVER_H

typedef struct rfileinfo rfileinfo;


struct rfileinfo
{
	char name[FILENMLEN];
	unsigned int size;
	struct prtdata *pd;
};



void receive_file(char *filename);
int collect_file(rfileinfo *finfo);
rfileinfo handle_notification();

#endif
