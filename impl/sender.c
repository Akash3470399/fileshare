#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "intr/sender.h"
#include "intr/ops.h"
#include "intr/const.h"
#include "intr/helper.h"
#include "intr/timer.h"
#include "intr/main.h"


// syntax : send <filename>
int isvalid_send_cmd(char *cmd)
{
	char str[16];
	int res = 0, tryno = 0, fileidx = 5;

	memmove(str, cmd, 4);			// coping 4 bytes as "send" has 4 bytes
	str[4] = '\0';

	if(strcmp(str, "send") == 0)
	{
		strcpy(str, &(cmd[fileidx]));
		while((tryno < NO_OF_TRIES) && (res == 0))
        {
		    if(isvalid_file(str) == 1)
			{	
				res = 1;
				strcpy(&(cmd[fileidx]), str);
			}
			else
			{
				printf("%s wrong\n", str);
				printf("Seems like you have enter wrong filename to send\n");
				printf("Enter file to send:");
				scanf("%s", str);
			}
			
			tryno += 1;
			if(tryno == NO_OF_TRIES)		// to show menu
				strcpy(cmd, "noneop\0");
		}
	}
	return res;
}


// handle send file operation
void send_file(char *filename)
{
	int res, filesize;
	unsigned char buf[BUFLEN];
	sfileinfo finfo;

	memmove(finfo.name, filename, FILENMLEN);
	finfo.size = get_filesize(filename);

	if(make_handshake(&finfo))
	{
		send_batchwise(&finfo);
	}
}

// check if receiver is requesting for correct next batch
int validate_batchreqst(unsigned int batchno)
{
	unsigned char op;
	unsigned int reqst_batch;
	int tryno = 0, resp = 0;
	timer *t = init_timer(_MSG_WAIT);

	while(tryno < NO_OF_TRIES && resp == 0)
	{
		rmsglen = receive_inbuffer(rbuf);
		op = rbuf[OPIDX];
		reqst_batch = bytestonum(&(rbuf[BTCIDX]));

		if(rmsglen > 0 && op == SENDBATCH && reqst_batch == batchno)
			resp = 1;

		if(timer_reached(t))
		{
			tryno += 1;
			reset_timer(t);
		}
	}

	return resp;
}

// Sends acknowledge to reciever that this sytem want to send a file
// & make appropiate settings 
// if receiver is ready to accept the file then notify_receiver function 
// return 1 else 0
// notify msg : <SENDING(1)><filename(FILENMLEN)><filesize(NUMSIZE)>
int make_handshake(sfileinfo *finfo)
{
	unsigned char op;
	unsigned int req_batch;
	int resp = 0, tryno = 0;

	timer *t = init_timer(_MSG_WAIT);

	// message create
	sbuf[OPIDX] = SENDING;
	memmove(&(sbuf[FLIDX]), finfo->name, FILENMLEN);
	numtobytes(&(sbuf[FILENMLEN + 1]), finfo->size);

	// length of message
	smsglen = 1 + FILENMLEN + NUMSIZE;
	
	// sending message
	send_buffer(sbuf, smsglen);

	while(tryno < NO_OF_TRIES && resp == 0)
	{
		rmsglen = receive_inbuffer(rbuf);
		req_batch = bytestonum(&(rbuf[BTCIDX]));
		op = rbuf[OPIDX];

		// if request for batch 0 is received
		if(rmsglen > 0 && op == SENDBATCH && req_batch == 0)
			resp = 1;

		// if timer reached resend the message
		if(timer_reached(t))
		{
			send_buffer(sbuf, smsglen);
			reset_timer(t);
			tryno += 1;
		}
	}
	return resp;
}

int send_batchwise(sfileinfo *finfo)
{

	unsigned int totalbatches, curbatch = 0, resp = 0, tryno = 0;

	totalbatches = CEIL(finfo->size, BATCHSIZE);

	while(tryno < NO_OF_TRIES && curbatch <= totalbatches)
	{
		if(send_batch(finfo, curbatch) == 1)
		{
			curbatch += 1;
			tryno -= 1;
		}
		tryno += 1;
	}
}


int send_batch(sfileinfo *finfo, unsigned int curbatch)
{

	int resp = 0;
	char msg[100];
	unsigned int reltv_batch = curbatch % BATCHESPOSSB, batchpos, partno = 0;
	unsigned int partspossible;
	FILE *sendfp = fopen(finfo->name, "rb+");

	batchpos = curbatch * BATCHSIZE;
	sbuf[OPIDX] = concat_batchnop(reltv_batch, DATA);
	partspossible = PARTSINBATCH;

	if(sendfp != NULL)
	{
		fseek(sendfp, batchpos, SEEK_SET);
		while(!feof(sendfp) && partno < partspossible)
		{
			numtobytes(&(sbuf[PRTIDX]), partno);
			smsglen = 1 + NUMSIZE;
			smsglen += fread(&(sbuf[DATAIDX]), 1, PARTSIZE, sendfp);
			send_buffer(sbuf, smsglen);
			partno += 1;
		}
	}

	if(send_missing_parts(finfo, curbatch))
		resp = 1;

	return resp;

}

int send_missing_parts(sfileinfo *finfo, unsigned int curbatch)
{
	unsigned char op;
	int tryno = 0, resp = 0;
	unsigned int reqst_batch, reltv_batch = curbatch % BATCHESPOSSB;
	timer *t = init_timer(_MSG_WAIT);

	while(tryno < NO_OF_TRIES && resp == 0)
	{
		rmsglen = receive_inbuffer(rbuf);
		op = rbuf[OPIDX];
		reqst_batch = bytestonum(&(rbuf[BTCIDX]));

		if(rmsglen > 0 && op == SENDBATCH && reqst_batch == curbatch +1)
			resp = 1;

		if(timer_reached(t))
		{
			sbuf[OPIDX] = concat_batchnop(reltv_batch, DATA);
			snprintf(&(sbuf[1]), 64, "rsend %d", curbatch);
			send_buffer(sbuf, 11);
			reset_timer(t);
			tryno += 1;
		}
	}

	return resp;
}
