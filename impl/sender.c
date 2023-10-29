#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "intr/sender.h"
#include "intr/timer.h"
#include "intr/main.h"
#include "intr/ops.h"
#include "intr/helper.h"
#include "intr/consts.h"

void send_file(char *filename)
{
	int stop = 0;
	unsigned int curbatch = 0, totalbatch;
	sfileinfo finfo;

	memmove(finfo.name, filename, FILENMLEN);
	finfo.size = get_filesize(filename);

	totalbatch = CEIL(finfo.size, batchsize);
	if(build_scontext(&finfo) == 1)
	{
		while((curbatch < totalbatch) && (stop == 0))
		{
			if(send_batch(&finfo, curbatch))
				curbatch += 1;
			else
				stop = 1;
		}
	}
	else
		printf("Failed to build context\n");
}


int build_scontext(sfileinfo *finfo)
{
	int success = 0;
	int cur_attempt = 0, totalattempts = 3;
	unsigned int reqst_batch, expd_batch = 0;

	_sendContext(finfo);
	timer *t = init_timer(100);	// wait for 10sec

	// listen for resp SENDBATCH & if timeout then resend it
	while(cur_attempt < totalattempts && success == 0)
	{
		recvmsglen = recv_msg(recvbuf);
		op = get_op(recvbuf[OPidx]); 
		reqst_batch = bytestonum(&(recvbuf[BTCidx]));

		if(recvmsglen > 0 && op == SENDBATCH && reqst_batch == 0)
		{
			record_time(t);
			_adjustContext();
			success = 1;
		}
		else if(timer_reached(t))
		{
			cur_attempt += 1;
			send_msg(sendbuf, sendmsglen);
			reset_timer_offset(t, t->offset*2);
		}
	}
	destroy_timer(t);
	return success;
}

int send_batch(sfileinfo *finfo, unsigned int curbatch)
{
	int resp = 0;
	unsigned char partno = 0;
	FILE *sendfp = fopen(finfo->name, "rb");
	unsigned int batchpos = curbatch * batchsize;

	if(sendfp != NULL)
	{
		fseek(sendfp, batchpos, SEEK_SET);
		while(!feof(sendfp) && partno < PARTSINBATCH)
		{
			sendbuf[OPidx] = encode_part(partno);
			numtobytes(&(sendbuf[BTCidx]), curbatch);
			sendmsglen = 1 + NUMSIZE;
			sendmsglen += fread(&(sendbuf[DATAidx]), 1, partsize, sendfp);
			send_msg(sendbuf, sendmsglen);
			partno += 1;
		}
		
		fclose(sendfp);
		resp = send_missing_parts(finfo, curbatch);
	}
	else 
		printf("unble to load the file\n");

	return resp;
}

int send_missing_parts(sfileinfo *finfo, unsigned int curbatch)
{
	int resp = 0, tryno = 0, msgcnt = PARTSINBATCH, atmpt = 0;
	unsigned char partno;
	unsigned int reqst_batchno, batchpos, partpos;
	int fileops = msgcnt * 1;
	timer *t = init_timer(MSGMAX *(msgcnt) + fileops);
	FILE *sendfp = fopen(finfo->name, "rb");

	batchpos = batchpos * batchsize;

	while(atmpt < ATTEMPTS && resp == 0)
	{
		recvmsglen = recv_msg(recvbuf);
		op = get_op(recvbuf[OPidx]);
		reqst_batchno = bytestonum(&(recvbuf[BTCidx]));

		if(recvmsglen > 0 && op == SENDBATCH && reqst_batchno == (curbatch + 1))
		{
			printf("%u sent\n", curbatch);
			resp = 1;
		}
		else if(recvmsglen > 0 && op == RESENDPARTS && reqst_batchno == curbatch && sendfp != NULL)
		{
			printf("sending missing parts %d batch, parts ", curbatch);
			for(int i = DATAidx; i < recvmsglen; i++)
			{
				partno = recvbuf[i];
				printf("%d ", partno);
				partpos = batchpos + (partno * partsize);
				fseek(sendfp, partpos, SEEK_SET);

				sendbuf[OPidx] = encode_part(partno);
				numtobytes(&(sendbuf[BTCidx]), curbatch);
				sendmsglen = 1 + NUMSIZE;

				sendmsglen += fread(&(sendbuf[DATAidx]), 1, partsize, sendfp);
				send_msg(sendbuf, sendmsglen);
			}
			reset_timer(t);
			printf("\n");
		}
		else if(timer_reached(t))
		{
			reset_timer_offset(t, t->offset*2);
			atmpt += 1;
		}
	}

	destroy_timer(t);
	if(sendfp != NULL) fclose(sendfp);
	return resp;
}

// create message to share context & share it once.
// msg : <SENDING(1)><filename(FILENMLEN)><size(NUMSIZE)><partsize(NUMSIZE)>
void _sendContext(sfileinfo *finfo)
{
	int prtidx, sizeidx; 

	sizeidx = FLidx + FILENMLEN;
	prtidx = sizeidx + NUMSIZE;
	// creating message
	sendbuf[OPidx] = SENDING;
	memmove(&(sendbuf[FLidx]), finfo->name, FILENMLEN);
	numtobytes(&(sendbuf[sizeidx]), finfo->size);		// filesize
	numtobytes(&(sendbuf[prtidx]), partsize);			// data part size in one message possible
					
	sendmsglen = 1 + FILENMLEN + NUMSIZE + NUMSIZE;		// message size
	send_msg(sendbuf, sendmsglen);
}

void _adjustContext()
{

	unsigned int r_partsize;
	int partidx = BTCidx + NUMSIZE;

	r_partsize = bytestonum(&(recvbuf[partidx]));
	if(partsize != r_partsize)
	{
		partsize = r_partsize;
		batchsize = partsize *PARTSINBATCH;		
	}
}


