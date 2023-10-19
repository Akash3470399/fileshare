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
	char recfile[FILENMLEN+5];
	rfileinfo finfo;	// fileinfo contains filename & size
	int n = 0, res, expt_data = 0; 
	FILE *fp;
	if(resp_handshake(&finfo))
	{

		receive_batchwise(&finfo);	
	}
	else 
		printf("unable to receive at this moment.\n");
}

// sending file with size 0 is not allowed
int resp_handshake(rfileinfo *finfo)
{
	FILE *fp;
	char filename[FILENMLEN+5];
	int resp = 0, tryno = 0, sizeidx = 1 + FILENMLEN;
	timer *t = init_timer(_RECV_WAIT);

	while(tryno < NO_OF_TRIES && resp == 0)
	{
		rmsglen = receive_inbuffer(rbuf);
		op = get_op(rbuf[OPIDX]);

		if(rmsglen > 0 && op == SENDING)
		{
			finfo->size = bytestonum(&(rbuf[sizeidx]));
			memmove(finfo->name, &(rbuf[FLIDX]), FILENMLEN);
			resp = 1;	
		}
		else if(rmsglen > 0)
			printf("0:%x, 1 :%x\n", rbuf[0], rbuf[1]);

		if(timer_reached(t))
		{
			reset_timer(t);
			tryno += 1;
		}
	}

	if(resp)
	{
		snprintf(filename, FILENMLEN+5, "rec/%s", finfo->name);
		if((fp = fopen(filename, "wb+")) != NULL)
			fclose(fp);
		finfo->pd = init_prtdata(PARTSINBATCH);
		resp = reqst_batch(finfo, 0);
	}
	else
		printf("failed to connect.\n");
	
	fflush(stdout);
	return resp;
}

// requests to sender to send batchno batch
// in response sender will send data from that batch 
int reqst_batch(rfileinfo *finfo, unsigned int batchno)
{
	FILE *fp;
	char filename[FILENMLEN+5];
	int resp = 0, tryno = 0;
	unsigned int resp_batchno, resp_partno;
	timer *t = init_timer(_MSG_WAIT * 4);

	snprintf(filename, FILENMLEN+5, "rec/%s", finfo->name);
	fp = fopen(filename, "rb+");
	
	sbuf[OPIDX] = SENDBATCH;
	numtobytes(&(sbuf[BTCIDX]), batchno);

	smsglen = 1 + NUMSIZE;
	send_buffer(sbuf, smsglen);
	
	printf("requested %d\n", batchno);
	while(tryno < NO_OF_TRIES && resp == 0)
	{
		rmsglen = receive_inbuffer(rbuf);
		// extracting inforamation from message
		op = get_op(rbuf[OPIDX]);
		resp_batchno = bytestonum(&(rbuf[BTCIDX]));
		resp_partno = get_partno(rbuf[OPIDX]); 
		
		if(rmsglen > 0 && op == DATA && resp_batchno == batchno)
		{
			writetofile(fp, batchno);
			set(finfo->pd, resp_partno);
			resp = 1;
		}
		
		if(timer_reached(t))
		{
			send_buffer(sbuf, smsglen);
			reset_timer(t);
			tryno += 1;
		}
	}
	if(fp != NULL) fclose(fp);
	return resp;
}

// write batch part to file
// also mark that part is received
int writetofile(FILE *recvfp, unsigned int batchno)
{
	unsigned char partno;
	unsigned int partpos, batchpos, wrtbyt = 0;
	char filename[FILENMLEN+5];

	// to calculate the position of received bytes in file
	partno = get_partno(rbuf[OPIDX]);
	batchpos = batchno * BATCHSIZE;
	partpos = batchpos + (partno * PARTSIZE);

	if(recvfp != NULL)
	{
		fseek(recvfp, partpos, SEEK_SET);
		wrtbyt = fwrite(&(rbuf[DATAIDX]), 1, rmsglen - 5, recvfp);	// 5 is header size
		fflush(recvfp);
	}
	return wrtbyt;
}

int receive_batchwise(rfileinfo *finfo)
{
	int tryno = 0, resp = 0;
	unsigned int curbatch = 0, totalbatches;
	
	totalbatches = CEIL(finfo->size, BATCHSIZE);
	printf("total batches %d\n", totalbatches);
	while(curbatch < totalbatches && resp == 0)
	{
		if(receive_batch(finfo, curbatch))
		{
			curbatch += 1;
			tryno = 0;
		}
		else
		{
			printf("unable to receive %d batch\n", curbatch);
			resp = 1;
		}
	}
	return resp;
}

int receive_batch(rfileinfo *finfo, unsigned int curbatch)
{
	char filename[FILENMLEN + 5];
	int tryno = 0, resp = 0, msgcnt = PARTSINBATCH;
	timer *t = init_timer(_MSG_WAIT * (msgcnt / 20));	// wait for at least 20% msg can reach
	FILE *recvfp;
	unsigned int resp_batchno, resp_partno, recved_prtcnt = 0;

	snprintf(filename, FILENMLEN+5, "rec/%s", finfo->name);
	recvfp = fopen(filename, "rb+");

	while(!(timer_reached(t)) && (recved_prtcnt < PARTSINBATCH) && recvfp != NULL)
	{
		rmsglen = receive_inbuffer(rbuf);
		op = get_op(rbuf[OPIDX]);
		resp_partno = get_partno(rbuf[OPIDX]);
		resp_batchno = bytestonum(&(rbuf[BTCIDX]));

		if(rmsglen > 0 && op == DATA && resp_batchno == curbatch)
		{
			writetofile(recvfp, curbatch);
			set(finfo->pd, resp_partno);
			reset_timer_offset(t, (_MSG_WAIT*3));
			recved_prtcnt += 1;
		}
	}
	resp = recover_parts(finfo, curbatch);
	return resp;
}

// format <RESENDPARTS(1)><partno(4)><partno(4)>...
// return size of request message
unsigned int create_missing_part_request(unsigned char *part_arr, unsigned int arrsize, unsigned int batchno)
{
	int buflimit = PARTSIZE, i;
	sbuf[OPIDX] = RESENDPARTS;
	numtobytes(&(sbuf[BTCIDX]), batchno);

	for(i = 0; i < arrsize && i < buflimit; i++)
	{
		sbuf[DATAIDX + i] = part_arr[i];
	}
	return (i+DATAIDX);
}

int recover_parts(rfileinfo *finfo, unsigned int curbatch)
{
	char filename[FILENMLEN+5];
	int resp = 0, missprt_cnt = 0, tryno = 0;
	unsigned char missparts_arr[PARTSINBATCH];
	unsigned int resp_partno, resp_batchno;
	timer *t = init_timer(_MSG_WAIT * ((PARTSINBATCH)/20));
	FILE *recvfp;

	snprintf(filename, FILENMLEN+5, "rec/%s", finfo->name);
	recvfp = fopen(filename, "rb+");
	missprt_cnt = getmisprts(finfo->pd, missparts_arr);

/*	if(missprt_cnt > 0)
	{	
		smsglen = create_missing_part_request(missparts_arr, missprt_cnt, curbatch);
		send_buffer(sbuf, smsglen);
	}
*/
	while(missprt_cnt > 0 && tryno < NO_OF_TRIES && recvfp != NULL)
	{
		rmsglen = receive_inbuffer(rbuf);
		op = get_op(rbuf[OPIDX]);
		resp_partno = get_partno(rbuf[OPIDX]);
		resp_batchno = bytestonum(&(rbuf[BTCIDX]));

		if(rmsglen > 0 && op == DATA && resp_batchno == curbatch)
		{
			writetofile(recvfp, curbatch);
			set(finfo->pd, resp_partno);
			reset_timer(t);
			tryno = 0;
		}

		missprt_cnt = getmisprts(finfo->pd, missparts_arr);
		
		if(timer_reached(t) && missprt_cnt > 0)
		{
			smsglen = create_missing_part_request(missparts_arr, missprt_cnt, curbatch);
			send_buffer(sbuf, smsglen);
			tryno += 1;
			reset_timer(t);
			//reset_timer_offset(t, (_MSG_WAIT * (smsglen/20)));
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
	
	if(recvfp != NULL) fclose(recvfp);
	return resp;
}
