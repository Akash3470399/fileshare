
#ifndef GLOBALS_H
#define GLOBALS_H
extern unsigned int selfport, recvport;
extern unsigned int partsize;
extern unsigned int batchsize;

extern unsigned int buflen;

void init_globals(int p1, int p2);
void show_globals();
#endif
