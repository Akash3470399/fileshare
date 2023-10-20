#include <stdio.h>
#include "intr/consts.h"

unsigned int selfport;
unsigned int recvport;
unsigned int partsize;
unsigned int batchsize;
unsigned int buflen;

void init_globals(int p1, int p2)
{
	selfport = p1;
	recvport = p2;
	partsize = 1024; 
	batchsize = partsize * PARTSINBATCH;
	buflen = partsize + 10;
}

void show_globals()
{
	printf("self port %u\n", selfport);
	printf("receiver port %u\n", recvport);
	printf("partsize %u\n", partsize);
	printf("batchsize %u\n", batchsize);
	printf("buflen %u\n", buflen);

}
