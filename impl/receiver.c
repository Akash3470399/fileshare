#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "intr/receiver.h"
#include "intr/timer.h"
#include "intr/main.h"
#include "intr/globals.h"
#include "intr/ops.h"
#include "intr/helper.h"
#include "intr/consts.h"

void receive_file()
{
	unsigned int curbatch = 0, totalbatch;
	int stop = 0;

	rfileinfo finfo;
	memmove(finfo.name, "recv/", 6);
	
	if(build_rcontext(&finfo))
	{
		totalbatch = CEIL(finfo.size, batchsize);
		if(reqst_firstbatch(&finfo))
		{
			while((curbatch < totalbatch) && stop == 0)
			{
				if(receive_batch(&finfo, curbatch))
					curbatch += 1;
				else
				{
					stop = 1;
				}

			}
		}
	}
	else
		printf("building context failed\n");
}


int build_rcontext(rfileinfo *finfo)
{
	FILE *fp;
	int success = 0, cur_attempt = 0, total_attempts = 3;
	timer *t = init_timer(60000);

	while((!timer_reached(t)) && (success == 0))
	{
		recvmsglen = recv_msg(recvbuf);
		op = get_op(recvbuf[OPidx]);

		if(op == SENDING)
		{
			_saveContext(finfo);	
			success = 1;
		}
	}

	destroy_timer(t);
	return success;
}

int reqst_firstbatch(rfileinfo *finfo)
{
	int partidx = BTCidx + NUMSIZE, cur_attempt = 0, total_attempts = ATTEMPTS, success = 0;
	unsigned int curbatch, recv_batch, partno;
	timer *t;
	FILE *recvfp;
	
	// creating request message
	sendbuf[OPidx] = SENDBATCH;
	numtobytes(&(sendbuf[BTCidx]), 0);
	numtobytes(&(sendbuf[partidx]), partsize);
	
	sendmsglen = 1 + NUMSIZE + NUMSIZE;

	t = init_timer(1000);
	send_msg(sendbuf, sendmsglen);

	while((cur_attempt < total_attempts) && success == 0)
	{
		recvmsglen = recv_msg(recvbuf);
		op = get_op(recvbuf[OPidx]);
		recv_batch = bytestonum(&(recvbuf[BTCidx]));
		partno = get_partno(recvbuf[PRTidx]);

		if(op == DATA && recv_batch == 0)
		{
			record_time(t);
			if((recvfp = fopen(finfo->name, "wb+")) != NULL)
			{
				finfo->pd = init_prtdata(partsize);
				writetofile(recvfp, 0);
				set(finfo->pd, partno);
				fclose(recvfp);
				success = 1;
			}
			else
				printf("file creation failed\n");
		}
		else if(timer_reached(t))
		{
			send_msg(sendbuf, sendmsglen);
			reset_timer(t);
			cur_attempt += 1;
		}
	}
	return success;
}

int reqst_batch(rfileinfo *finfo, unsigned int batchno)
{
	int partidx = BTCidx + NUMSIZE, cur_attempt = 0, total_attempts = ATTEMPTS, success = 0;
	unsigned int curbatch, recv_batch, partno;
	timer *t;
	FILE *recvfp;
	
	// creating request message
	sendbuf[OPidx] = SENDBATCH;
	numtobytes(&(sendbuf[BTCidx]), batchno);
	
	sendmsglen = 1 + NUMSIZE;

	t = init_timer(MSGLAST);
	send_msg(sendbuf, sendmsglen);

	while((cur_attempt < total_attempts) && success == 0)
	{
		recvmsglen = recv_msg(recvbuf);
		op = get_op(recvbuf[OPidx]);
		recv_batch = bytestonum(&(recvbuf[BTCidx]));
		partno = get_partno(recvbuf[PRTidx]);

		if(op == DATA && recv_batch == batchno)
		{
			record_time(t);
			if((recvfp = fopen(finfo->name, "rb+")) != NULL)
			{
				finfo->pd = init_prtdata(partsize);
				writetofile(recvfp, 0);
				set(finfo->pd, partno);
				fclose(recvfp);
				success = 1;
			}
			else
				printf("file creation failed\n");
		}
		else if(timer_reached(t))
		{
			send_msg(sendbuf, sendmsglen);
			reset_timer_offset(t, t->offset*2);
			cur_attempt += 1;
		}
	}
	return success;
}
// write batch part to file
// also mark that part is received
int writetofile(FILE *recvfp, unsigned int batchno)
{
	unsigned char partno;
	unsigned int partpos, batchpos, wrtbyt = 0;
	char filename[FILENMLEN+5];

	// to calculate the position of received bytes in file
	partno = get_partno(recvbuf[PRTidx]);
	batchpos = batchno * batchsize;
	partpos = batchpos + (partno * partsize);

	if(recvfp != NULL)
	{
		fseek(recvfp, partpos, SEEK_SET);
		wrtbyt = fwrite(&(recvbuf[DATAidx]), 1, recvmsglen- 5, recvfp);	// 5 is header size
		fflush(recvfp);
		//write(1, &(recvbuf[DATAidx]), recvmsglen-5);
	}
	return wrtbyt;
}

int receive_batch(rfileinfo *finfo, unsigned int curbatch)
{
	char filename[FILENMLEN + 5];
	int tryno = 0, resp = 0, msgcnt = PARTSINBATCH;
	FILE *recvfp;
	unsigned int resp_batchno, resp_partno, recved_prtcnt = 0;

	timer *t = init_timer(MSGLAST * (msgcnt/5));	// wait for at least 20% msg can reach
	recvfp = fopen(finfo->name, "rb+");
	
	while(!(timer_reached(t)) && (recved_prtcnt < PARTSINBATCH) && recvfp != NULL)
	{
		recvmsglen = recv_msg(recvbuf);
		op = get_op(recvbuf[OPidx]);
		resp_partno = get_partno(recvbuf[OPidx]);
		resp_batchno = bytestonum(&(recvbuf[BTCidx]));

		if(recvmsglen > 0 && op == DATA && resp_batchno == curbatch)
		{
			writetofile(recvfp, curbatch);
			set(finfo->pd, resp_partno);

			int avg = (MSGMIN + MSGLAST)/2;
			reinit_timer(t, avg * 2);
			recved_prtcnt += 1;
		}
	}
	fclose(recvfp);
	destroy_timer(t);
	resp = recover_parts(finfo, curbatch);
	return resp;
}

// format <RESENDPARTS(1)><partno(4)><partno(4)>...
// return size of request message
unsigned int create_missing_part_request(unsigned char *part_arr, unsigned int arrsize, unsigned int batchno)
{
	int buflimit = partsize, i;
	sendbuf[OPidx] = RESENDPARTS;
	numtobytes(&(sendbuf[BTCidx]), batchno);

	for(i = 0; i < arrsize && i < buflimit; i++)
	{
		sendbuf[DATAidx + i] = part_arr[i];
	}
	return (i+DATAidx);
}

int recover_parts(rfileinfo *finfo, unsigned int curbatch)
{
	int resp = 0, missprt_cnt = 0, tryno = 0;
	unsigned char missparts_arr[PARTSINBATCH];
	unsigned int resp_partno, resp_batchno;
	
	timer *t = init_timer(MSGLAST * 2);
	FILE *recvfp;

	recvfp = fopen(finfo->name, "rb+");
	missprt_cnt = getmisprts(finfo->pd, missparts_arr);

	while(missprt_cnt > 0 && tryno < ATTEMPTS && recvfp != NULL)
	{
		recvmsglen =recv_msg(recvbuf);
		op = get_op(recvbuf[OPidx]);
		resp_partno = get_partno(recvbuf[OPidx]);
		resp_batchno = bytestonum(&(recvbuf[BTCidx]));

		if(recvmsglen > 0 && op == DATA && resp_batchno == curbatch)
		{
			writetofile(recvfp, curbatch);
			set(finfo->pd, resp_partno);
			reinit_timer(t, MSGLAST);
			tryno = 0;
		}

		missprt_cnt = getmisprts(finfo->pd, missparts_arr);
		
		if(timer_reached(t) && missprt_cnt > 0)
		{
			sendmsglen = create_missing_part_request(missparts_arr, missprt_cnt, curbatch);
			send_msg(sendbuf, sendmsglen);
			tryno += 1;
			reinit_timer(t, MSGLAST * (PARTSINBATCH/5));
		}
	}
		
	if(missprt_cnt == 0)
	{
		fflush(stdout);
		printf("%u received\n", curbatch);
		fflush(stdout);
		destroy_prddata(finfo->pd);
		finfo->pd = init_prtdata(PARTSINBATCH);
		resp = reqst_batch(finfo, curbatch +1);
	}
	destroy_timer(t);
	if(recvfp != NULL) fclose(recvfp);
	return resp;
}

void _saveContext(rfileinfo *finfo)
{
	int partidx, sizeidx;
	int s_partsize;

	sizeidx = FLidx + FILENMLEN;
	partidx = sizeidx + NUMSIZE;
	
	memmove(&(finfo->name[5]), &(recvbuf[FLidx]), FILENMLEN);
	finfo->size = bytestonum(&(recvbuf[sizeidx]));
	s_partsize = bytestonum(&(recvbuf[partidx]));

	// sender has smaller partsize then
	// adjust accordingly
	if(s_partsize < partsize)
	{
		partsize = s_partsize;
		buflen = partsize + 10;
		batchsize = partsize * PARTSINBATCH;
		free(sendbuf), free(recvbuf);

		sendbuf = (char *)malloc(buflen);
		recvbuf = (char *)malloc(buflen);
	}
}
