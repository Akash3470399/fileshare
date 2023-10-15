#include "intr/const.h"

#ifndef MAIN_H
#define MAIN_H


extern unsigned char sbuf[BUFLEN];
extern unsigned int smsglen;

extern unsigned char rbuf[BUFLEN];
extern unsigned int rmsglen;

extern unsigned char op;


int receive_inbuffer(unsigned char *buffer);
int send_buffer(unsigned char *buffer, int len);
#endif
