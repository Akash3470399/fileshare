
#ifndef MAIN_H
#define MAIN_H

extern unsigned int sendmsglen;
extern unsigned int recvmsglen;

extern unsigned char *sendbuf;
extern unsigned char *recvbuf;

extern unsigned char op;

int send_msg(unsigned char *msg, int msglen);
int recv_msg(unsigned char *msg);
#endif
