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
// file name should not have spaces
int isvalid_send_cmd(char *cmd)
{
	char str[FILENMLEN];
	int res = 0, tryno = 0, fileidx = 5;

	memmove(str, cmd, 4);			// coping 4 bytes as "send" has 4 bytes
	str[4] = '\0';

	if(strcmp(str, "send") == 0)
	{
		strncpy(str, &(cmd[fileidx]), FILENMLEN);
		while((tryno < NO_OF_TRIES) && (res == 0))
        {
		    if(isvalid_file(str) == 1)
			{	
				res = 1;
				strncpy(&(cmd[fileidx]), str, FILENMLEN);
			}
			else
			{
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

	// file metadata
	memmove(finfo.name, filename, FILENMLEN);
	finfo.size = get_filesize(filename);

	if(make_handshake(&finfo))
	{
		printf("done handshake");
		send_batchwise(&finfo);
	}
	else
		printf("Unable to share at this moment.\n");
}

// Sends acknowledge to reciever that this sytem want to send a file
// & make appropiate settings 
// if receiver is ready to accept the file then notify_receiver function 
// return 1 else 0
// notify msg : <SENDING(1)><filename(FILENMLEN)><filesize(NUMSIZE)>
int make_handshake(sfileinfo *finfo)
{
	int sizeidx = 1 + FILENMLEN, resp = 0, tryno = 0, expd_batch = 0, reqst_batchno;
	timer *t = init_timer(_RECV_WAIT);

	// forming message
	sbuf[OPIDX] = SENDING;
	memmove(&(sbuf[FLIDX]), finfo->name, FILENMLEN);
	numtobytes(&(sbuf[sizeidx]), finfo->size);

	smsglen = 1 + FILENMLEN + NUMSIZE;

	send_buffer(sbuf, smsglen);

	while(tryno < NO_OF_TRIES && resp == 0)
	{
		rmsglen = receive_inbuffer(rbuf);
		op = get_op(rbuf[OPIDX]);
		reqst_batchno = bytestonum(&(rbuf[BTCIDX]));

		if(rmsglen > 0 && op == SENDBATCH && reqst_batchno == expd_batch)
			resp = 1;

		if(timer_reached(t))
		{	// resending message
			send_buffer(sbuf, smsglen);
			reset_timer(t);
			tryno += 1;
		}
	}
	
	// reseting buffer
	memset(rbuf, 0, sizeof(rbuf));	
	rmsglen = 0;

	return resp;
}

int send_batchwise(sfileinfo *finfo)
{
	unsigned int curbatch = 0, totalbatches;
	int tryno = 0, resp = 0;
	timer *t = init_timer(_MSG_WAIT);

	totalbatches = CEIL(finfo->size, BATCHSIZE);

	while(tryno < NO_OF_TRIES && curbatch < totalbatches)
	{
		printf("curbatch %d\n", curbatch);
		if(send_batch(finfo, curbatch))
		{
			curbatch += 1;	
			tryno = 0;
			reset_timer(t);
		}	
		
		if(timer_reached(t))
		{
			tryno += 1;
			reset_timer(t);
		}
	}

	return resp;
}


int send_batch(sfileinfo *finfo, unsigned int curbatch)
{
	int resp = 0;
	unsigned char partno = 0;
	FILE *sendfp = fopen(finfo->name, "rb");

	printf("batch no %d\n", curbatch);
	while(!feof(sendfp) && partno < PARTSINBATCH)
	{
		sbuf[OPIDX] = encode_part(partno);
		numtobytes(&(sbuf[BTCIDX]), curbatch);
		smsglen = 1 + NUMSIZE;

		smsglen += fread(&(sbuf[DATAIDX]), 1, PARTSIZE, sendfp);

		send_buffer(sbuf, smsglen);
		partno += 1;
	}

	resp = send_missing_parts(finfo, curbatch);

	return resp;
}

int send_missing_parts(sfileinfo *finfo, unsigned int curbatch)
{
	int resp = 0, tryno = 0;
	unsigned int reqst_batchno;
	timer *t = init_timer(_RECV_WAIT);

	while(tryno < NO_OF_TRIES && resp == 0)
	{
		rmsglen = receive_inbuffer(rbuf);
		op = get_op(rbuf[OPIDX]);
		reqst_batchno = bytestonum(&(rbuf[BTCIDX]));

		if(rmsglen > 0 && op == SENDBATCH && reqst_batchno == (curbatch + 1))
			resp = 1;
		else if(rmsglen > 0)
			printf("0:%x reqst_batchno %d cur batch %d\n", rbuf[0], reqst_batchno, curbatch);
	
		if(timer_reached(t))
		{
			reset_timer(t);
			tryno += 1;
		}
	}
	return resp;
}
