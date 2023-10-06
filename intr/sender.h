#include "intr/const.h"

#ifndef SENDER_H
#define SENDER_H

typedef struct fileinfo fileinfo;

struct fileinfo
{
	char name[FILENMLEN];
	unsigned int size;
};


int isvalid_send_cmd(char *cmd);

int send_nextbatch(fileinfo *finfo, int batches_sent);
int send_batchwise(fileinfo *finfo);
void send_file(char *filename);
int notify_receiver(fileinfo *finfo);
#endif
