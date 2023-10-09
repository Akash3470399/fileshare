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

int send_batchwise(fileinfo *finfo);
int send_batch(fileinfo *finfo, unsigned int batches_sent, unsigned int batchno);
int send_missing_parts(FILE *sendfilefp, long batchpos, unsigned int batchno);
void send_file(char *filename);
int batch_recovery(fileinfo *finfo, int cur_batch_cycle, int batchno);
int notify_receiver(fileinfo *finfo);
#endif
