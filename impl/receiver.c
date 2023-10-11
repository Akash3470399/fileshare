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
		receive_batchwise(&finfo);	
	}
	printf("expcted file size %d\n", finfo.size);
}

// sending file with size 0 is not allowed
rfileinfo respto_handshake()
{
	unsigned char op;
	rfileinfo finfo;
	int tryno = 0, resp = 0;
	timer *t = init_timer(_RECV_WAIT);

	finfo.name[0] = 'r';
	finfo.size = 0;

	while(tryno < NO_OF_TRIES && resp == 0)
	{
		rmsglen = receive_inbuffer(rbuf);
		op = rbuf[OPIDX];
		if(rmsglen > 0 && op == SENDING)
		{
			memmove(&(finfo.name[1]), &(rbuf[FLIDX]), FILENMLEN - 1);
			finfo.size = bytestonum(&(rbuf[FILENMLEN+1]));
			resp = 1;
		}

		if(timer_reached(t))
		{
			tryno += 1;
			reset_timer(t);
		}
	}

	if(resp == 1)
		req_batch(&finfo, 0);

	destroy_timer(t);
	return finfo;
}


int req_batch(rfileinfo *finfo, unsigned int batchno)
{
	unsigned char op;
	int tryno = 0,resp = 0, reltv_batch;
	timer *t = init_timer(_MSG_WAIT);
	
	reltv_batch = batchno % BATCHESPOSSB; 
	
	sbuf[OPIDX] = SENDBATCH;
	numtobytes(&(sbuf[BTCIDX]), batchno);
	smsglen = 1 + NUMSIZE;
	
	send_buffer(sbuf, smsglen);

	while(tryno < NO_OF_TRIES && resp == 0)
	{
		rmsglen = receive_inbuffer(rbuf);
		op = get_op(rbuf[OPIDX]);
		if(rmsglen > 0 && op == DATA && reltv_batch == get_batchno(rbuf[OPIDX]))
		{
			resp = 1;
			write(1, &rbuf[1], 10);
		}

		if(timer_reached(t))
		{
			send_buffer(sbuf, smsglen);
			reset_timer(t);
			tryno += 1;
		}
	}
	return resp;
}

// write batch part to file
int writetofile(FILE *recvfp, unsigned int batchno)
{
	unsigned int batchpos, partpos, partno, totalpart, wrtbyte = 0;
	
	batchpos = batchno * BATCHSIZE;
	partno = bytestonum(&(rbuf[PRTIDX]));
	partpos = batchpos + (partno * PARTSIZE);
	
	if(recvfp != NULL)
	{
		fseek(recvfp, partpos, SEEK_SET);
		//set(finfo->pd, partno);
		wrtbyte = fwrite(&(rbuf[DATAIDX]), 1, rmsglen - 5, recvfp);	// 5 is header size 
		fflush(recvfp);
		fclose(recvfp);
	}
	else
		printf("write file failed\n");
	return wrtbyte;
}

int receive_batchwise(rfileinfo *finfo)
{
	unsigned int totalbatches, curbatch = 0, resp = 0, tryno = 0;

	totalbatches = 0xFFFF;//CEIL(finfo->size, BATCHSIZE);

	while(tryno < NO_OF_TRIES && curbatch <= totalbatches)
	{
		if(receive_batch(finfo, curbatch))
		{
			curbatch += 1;
			tryno -= 1;
		}
		tryno += 1;
	}
}

int receive_batch(rfileinfo *finfo, unsigned int curbatch)
{
	unsigned char op;
	int tryno = 0, reltv_batch = curbatch % BATCHESPOSSB, resp = 0;
	FILE *recvfp = fopen(finfo->name, "wb+");
	unsigned int batchpos = curbatch * BATCHSIZE, partno, partpos;
	timer *t = init_timer(_MSG_WAIT);

	if(recvfp != NULL)
	{
		while(tryno < NO_OF_TRIES)
		{
			rmsglen = receive_inbuffer(rbuf);
			op = get_op(rbuf[OPIDX]);
			
			if(rmsglen > 0 && op == DATA && get_batchno(rbuf[OPIDX]) == reltv_batch)
			{
				writetofile(recvfp, curbatch);
				reset_timer(t);
			}

			if(timer_reached(t))
			{	
				tryno += 1;
				reset_timer(t);
			}
		}
		fclose(recvfp);
		resp = recover_parts(finfo, curbatch);
	}
	return resp;
}

int recover_parts(rfileinfo *finfo, unsigned int curbatch)
{
	int resp = 0;
	printf("received %d\n", curbatch);
	resp = req_batch(finfo, curbatch+1);
	if(resp > 0)
		resp = 1;
	return resp;
}
