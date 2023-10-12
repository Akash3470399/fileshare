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
	FILE *recvfp = fopen(finfo->name, "wb+");
	unsigned int partinbatch = PARTSINBATCH;

	reltv_batch = batchno % BATCHESPOSSB; 
	
	sbuf[OPIDX] = SENDBATCH;
	numtobytes(&(sbuf[BTCIDX]), batchno);
	smsglen = 1 + NUMSIZE;
	
	send_buffer(sbuf, smsglen);
	if(recvfp != NULL)
	{
		while(tryno < NO_OF_TRIES && resp == 0)
		{
			rmsglen = receive_inbuffer(rbuf);
			op = get_op(rbuf[OPIDX]);
			if(rmsglen > 0 && op == DATA && reltv_batch == get_batchno(rbuf[OPIDX]))
			{
				finfo->pd = init_prtdata(partinbatch);
				resp = 1;
				writetofile(finfo, recvfp, batchno);
			}
			else if(timer_reached(t))
			{
				send_buffer(sbuf, smsglen);
				reset_timer(t);
				tryno += 1;
			}
		}
		fclose(recvfp);
	}
	return resp;
}

// write batch part to file
int writetofile(rfileinfo *finfo, FILE *recvfp, unsigned int batchno)
{
	unsigned int batchpos, partpos, partno, totalpart, wrtbyte = 0;
	
	batchpos = batchno * BATCHSIZE;
	partno = bytestonum(&(rbuf[PRTIDX]));
	partpos = batchpos + (partno * PARTSIZE);

	printf("batch %d part %d partpos %d\n", batchno, partno, partpos);
	int fd = open("rx", O_CREAT | O_RDWR, 0777);
	if(recvfp != NULL)
	{
		set(finfo->pd, partno);
		fseek(recvfp, partpos, SEEK_SET);
		//set(finfo->pd, partno);
		wrtbyte = fwrite(&(rbuf[DATAIDX]), 1, rmsglen - 5, recvfp);	// 5 is header size 
																
		fflush(recvfp);
		write(fd, &(rbuf[DATAIDX]), rmsglen -5);
		fflush(recvfp);
		close(fd);
	}
	else
		printf("write file failed\n");
	return wrtbyte;
}

int receive_batchwise(rfileinfo *finfo)
{
	unsigned int totalbatches, curbatch = 0, resp = 0, tryno = 0;
	timer *t = init_timer(_RECV_WAIT);
	totalbatches = CEIL(finfo->size, BATCHSIZE);

	while(!timer_reached(t) && curbatch < totalbatches)
	{
		if(receive_batch(finfo, curbatch) == 1)
		{
			curbatch += 1;
			reset_timer(t);
		}
	}
}

int receive_batch(rfileinfo *finfo, unsigned int curbatch)
{
	unsigned char op;
	int tryno = 0, reltv_batch = curbatch % BATCHESPOSSB, resp = 0;
	FILE *recvfp = fopen(finfo->name, "wb+");
	unsigned int batchpos = curbatch * BATCHSIZE, partno, partinbatch;
	timer *t = init_timer(_MSG_WAIT);

	partinbatch = PARTSINBATCH;
	if(recvfp != NULL)
	{
		while(tryno < NO_OF_TRIES)
		{
			rmsglen = receive_inbuffer(rbuf);
			op = get_op(rbuf[OPIDX]);
			
			if(rmsglen > 0 && op == DATA && get_batchno(rbuf[OPIDX]) == reltv_batch)
			{
				partno = bytestonum(&(rbuf[PRTIDX]));
				set(finfo->pd, partno);
				writetofile(finfo, recvfp, curbatch);
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

unsigned int create_missing_part_request(int *part_arr, unsigned int arrsize, unsigned int batchno)
{
	int buflimit = PARTSIZE / (NUMSIZE);	// this is how many part numbers can be requested in one request
	int prtidx = 1;
	
	sbuf[OPIDX] = RESENDPARTS;	
   	for(int i = 0; i < arrsize && i < buflimit; i++)
	{
		numtobytes(&(sbuf[prtidx]), part_arr[i]);
		prtidx += NUMSIZE;
	}
	return prtidx;
}

int recover_parts(rfileinfo *finfo, unsigned int curbatch)
{
	unsigned char op;
	int resp = 0, missing_parts[PARTSINBATCH], missing_prtcnt, tryno = 0;
	unsigned int reltv_batch = curbatch % BATCHESPOSSB;
	timer *t = init_timer(_MSG_WAIT);
	FILE *sendfp;

	missing_prtcnt = getmisprts(finfo->pd, missing_parts);

	while(missing_prtcnt > 0 && tryno < NO_OF_TRIES)
	{
		if(timer_reached(t))
		{
			smsglen = create_missing_part_request(missing_parts, missing_prtcnt, curbatch);
			send_buffer(sbuf, smsglen);
			printf("batch %d missing %d\n", curbatch, missing_prtcnt);
			tryno += 1;
			reset_timer(t);
		}

		rmsglen = receive_inbuffer(rbuf);
		op = get_op(rbuf[OPIDX]);
		if(rmsglen > 0 && op == DATA && get_batchno(rbuf[OPIDX]) == reltv_batch)
		{
			sendfp = fopen(finfo->name, "wb+");
			if(sendfp != NULL)
			{
				writetofile(finfo, sendfp, curbatch);
				fclose(sendfp);
			}
			reset_timer(t);
			missing_prtcnt = getmisprts(finfo->pd, missing_parts);
		}
	}
	
	if(missing_prtcnt == 0) 
	{
		destroy_prddata(finfo->pd);
		resp = req_batch(finfo, curbatch+1);
	}
	
	resp = req_batch(finfo, curbatch+1);
	if(resp > 0)
		resp = 1;
	return resp;
}
