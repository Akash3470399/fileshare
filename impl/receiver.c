#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#include "intr/receiver.h"
#include "intr/helper.h"
#include "intr/ops.h"
#include "intr/const.h"
#include "intr/main.h"
#include "intr/timer.h"
#include "intr/part.h"



// handles if sender want tovoid receive_file(char *filename)
void receive_file(char *filename)
{
	rfileinfo finfo;	// fileinfo contains filename & size
	int n = 0, res, expt_data = 0; 
	
	finfo = respto_handshake();			 

	if(finfo.size > 0)
	{
		if(req_batch(&finfo, 0) == 1)
			receive_batchwise(&finfo);	
	}
	printf("expcted file size %d\n", finfo.size);

}


rfileinfo respto_handshake()
{
	int resp = 0;
	rfileinfo finfo;
	timer *t = init_timer(_RECV_WAIT);
	
	finfo.size = -1;
	strncpy(finfo.name, "rec/", 4);

	while(!(timer_reached(t)) && resp == 0)
	{
		rmsglen = receive_inbuffer(rbuf);
		if(rmsglen > 0 && rbuf[OPIDX] == SENDING)
		{
			memmove(&(finfo.name[4]), &(rbuf[FLIDX]), FILENMLEN - 4);
			finfo.size = bytestonum(&(rbuf[FILENMLEN+1]));
			printf("exp size %d ", finfo.size);
			resp = 1;
		}
		else if(rmsglen > 0)
			printf("op %x", rbuf[OPIDX]);
	}	
	return finfo;
}


int req_batch(rfileinfo *finfo, unsigned int batchno)
{
	int resp = 0;
	timer *t = init_timer(_RECV_WAIT);
	unsigned char opbt;	// operation & batch
	unsigned int batchpos, partno, partpos, totalpart;

	sbuf[OPIDX] = SENDBATCH;
	numtobytes(&(sbuf[BTCIDX]), batchno);
	
	smsglen = 1 + NUMSIZE;
	while(!(timer_reached(t)) && resp == 0)
	{
		send_buffer(sbuf, smsglen);
		opbt = rbuf[OPIDX];

		rmsglen = receive_inbuffer(rbuf);
		if(rmsglen > 0 && get_op(opbt) == DATA && get_batchno(opbt) == (batchno % BATCHESPOSSB))
		{	
			totalpart = PARTSINBATCH; // need to be update
			finfo->pd = init_prtdata(totalpart);
			if(writetofile(finfo, batchno) > 0)
				resp = 1;
			else
				printf("unable to write.\n");
		}
		else if(rmsglen > 0)
			printf("op %x not %x occured\n", opbt, DATA);
	}
	return resp;
}

// write batch part to file
int writetofile(rfileinfo *finfo, unsigned int batchno)
{
	unsigned int batchpos, partpos, partno, totalpart, wrtbyte = 0;
	FILE *recvfp;
	
	batchpos = batchno * BATCHSIZE;
	partno = bytestonum(&(rbuf[PRTIDX]));
	partpos = batchpos + (partno * PARTSIZE);
	
	recvfp = fopen(finfo->name, "wb+");
	
	if(recvfp != NULL)
	{
		fseek(recvfp, partpos, SEEK_SET);
		set(finfo->pd, partno);
		wrtbyte = fwrite(&(rbuf[DATAIDX]), 1, rmsglen - 5, recvfp);	// 5 is header size 
	//	write(1, &rbuf[DATAIDX], rmsglen-5);
		fflush(recvfp);
		fclose(recvfp);
	}
	else
		printf("write file failed\n");
	printf("recvd %d written %d\n", rmsglen-5, wrtbyte);
	return wrtbyte;
}


int receive_batchwise(rfileinfo *finfo)
{
	unsigned int batchno = 0, totalbatches, batches_recv = 0;
	timer *t = init_timer(_MSG_WAIT);
	totalbatches = CEIL(finfo->size, BATCHSIZE);

	while(!(timer_reached(t)) && batches_recv <= totalbatches)
	{
		if(receive_batch(finfo, batches_recv) == 1)
		{
			batches_recv += 1;
			reset_timer(t);
		}
		else
			printf("Failed to receive.\n");
	}
}

int receive_batch(rfileinfo *finfo, unsigned int batches_recv)
{
	unsigned char opbt;
	int resp = 0;
	timer *t = init_timer(_MSG_WAIT);
	unsigned int expd_batch = batches_recv % BATCHESPOSSB;
	while(!(timer_reached(t)))
	{
		rmsglen = receive_inbuffer(rbuf);
		opbt = rbuf[OPIDX];
		if(rmsglen > 0 && get_op(opbt) == DATA && get_batchno(opbt) == expd_batch)
		{
			writetofile(finfo, batches_recv);
			reset_timer(t);
		}
	}

	if(recover_parts(finfo, batches_recv) == 1)
		resp = 1;
	printf("resp %d for %d batch", resp, batches_recv);
	return resp;
}

int recover_parts(rfileinfo *finfo, unsigned int batches_recv)
{
	int resp = 0;
	
	resp = req_batch(finfo, batches_recv+1);
	if(resp > 0)
		resp = 1;
	return resp;
}
