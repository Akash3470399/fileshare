#include "intr/consts.h"

#ifndef SENDER_H
#define SENDER_H

typedef struct sfileinfo sfileinfo;

struct sfileinfo
{
	char name[FILENMLEN];
	unsigned int size;
};

void send_file(char *filename);
int build_scontext(sfileinfo *finfo);

int send_batch(sfileinfo *finfo, unsigned int curbatch);
int send_missing_parts(sfileinfo *finfo, unsigned int curbatch);



// internal methods
void _sendContext(sfileinfo *finfo);
void _adjustContext();
#endif
