#ifndef MAIN_H
#define MAIN_H

extern unsigned int selfport, recvport;
extern unsigned int partsize;
extern unsigned int batchsize;

extern unsigned int buflen;

void init_globals(int p1, int p2);
void show_globals();

extern unsigned int sendmsglen;
extern unsigned int recvmsglen;

extern unsigned char *sendbuf;
extern unsigned char *recvbuf;

extern unsigned char op;

int send_msg(unsigned char *msg, int msglen);
int recv_msg(unsigned char *msg);
#endif
