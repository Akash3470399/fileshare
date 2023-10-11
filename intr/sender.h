#include "intr/const.h"

#ifndef SENDER_H
#define SENDER_H

extern unsigned char sbuf[BUFLEN];
extern unsigned int smsglen;

extern unsigned char rbuf[BUFLEN];
extern unsigned int rmsglen;



typedef struct sfileinfo sfileinfo;
struct sfileinfo
{
	char name[FILENMLEN];
	unsigned int size;
};


int isvalid_send_cmd(char *cmd);
int make_handshake(sfileinfo *finfo);
int send_batchwise(sfileinfo *finfo);
int send_batch(sfileinfo *finfo, unsigned int batches_send);
int send_missing_parts(sfileinfo *finfo, unsigned int batches_send);
void send_file(char *filename);
#endif
