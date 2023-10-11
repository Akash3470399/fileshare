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

// Sends acknowledge to reciever that this sytem want to send a file
// & make appropiate settings 
// if receiver is ready to accept the file then notify_receiver function 
// return 1 else 0
// notify msg : <SENDING(1)><filename(FILENMLEN)><filesize(NUMSIZE)>
int make_handshake(sfileinfo *finfo)
{
	unsigned int req_batch;
	int resp = -1;

	timer *t = init_timer(_RECV_WAIT);

	memset(sbuf, 0, sizeof(sbuf));

	sbuf[OPIDX] = SENDING;
	memmove(&(sbuf[FLIDX]), finfo->name, FILENMLEN);
	numtobytes(&(sbuf[FILENMLEN + 1]), finfo->size);

	smsglen = 1 + FILENMLEN + NUMSIZE;

	while(!(timer_reached(t)) && resp == -1)
	{
		send_buffer(sbuf, smsglen);
		WAIT(5);		
		rmsglen = receive_inbuffer(rbuf);
		req_batch = bytestonum(&(rbuf[BTCIDX]));

		if(rmsglen > 0 && rbuf[OPIDX] == SENDBATCH && req_batch == 0)
			resp = 1;
		else if(rmsglen > 0)
			printf("handshake failed\n");
	}
	return resp;
}



int send_batchwise(sfileinfo *finfo)
{
	int res = 1;
	unsigned int batchno = 0, totalbatches, batches_send = 0, req_batch;
	timer *t = init_timer(_RECV_WAIT);
	totalbatches = CEIL(finfo->size, BATCHSIZE);
	while(!(timer_reached(t)) && batches_send <= totalbatches)
	{
		req_batch = send_batch(finfo, batches_send); 
		if(req_batch > -1)
		{
				batches_send += 1;
			reset_timer(t);
		}

	}
}


int send_batch(sfileinfo *finfo, unsigned int batches_send)
{
	unsigned char opbt;
	int resp = 0;
	unsigned int partno = 0, partspossible, partpos, batchno, batchpos;
	timer *t = init_timer(_MSG_WAIT);
	FILE *sendfp = fopen(finfo->name, "rb+");;


	batchno = batches_send % BATCHESPOSSB;
	partspossible = PARTSINBATCH;	// need to update
	batchpos = batches_send * BATCHSIZE;

	if(sendfp != NULL)
	{
		fseek(sendfp, batchpos, SEEK_SET);
		sbuf[OPIDX] = concat_batchnop(batchno, DATA);
		while(!(timer_reached(t)) && partno < partspossible)
		{
			numtobytes(&(sbuf[PRTIDX]), partno);
			smsglen = 1 + NUMSIZE;

			smsglen += fread(&(sbuf[DATAIDX]), 1, PARTSIZE, sendfp);
			send_buffer(sbuf, smsglen);
			partno += 1;
		}
		fclose(sendfp);

		if((resp = send_missing_parts(finfo, batches_send)) < 0)
			printf("send miss failed\n");
		else
			resp = 1;
				
	}
	return resp;
}

int send_missing_parts(sfileinfo *finfo, unsigned int batches_send)
{
	int resp = -1;
	timer *t = init_timer(_RECV_WAIT);
	unsigned int opbt, requested_batch;
	while(!(timer_reached(t)) && resp == -1)
	{
		rmsglen = receive_inbuffer(rbuf);
		opbt = rbuf[OPIDX];
		requested_batch = bytestonum(&(rbuf[BTCIDX]));
		if(rmsglen > 0 && rbuf[OPIDX] == SENDBATCH && requested_batch == batches_send +1) 
		{
			resp = 0;
			printf("handled missing parts\n");
		}
		else if(rmsglen > 0 && rbuf[OPIDX] == SENDBATCH)
		{
			printf("in missing %d exp but %d got\n", batches_send+1, requested_batch);
			//resp = requested_batch;
			resp = 1;
		}		else resp = -1;
	}

	return resp;
}
// handle send file operation
void send_file(char *filename)
{
	int res, filesize;
	unsigned char buf[BUFLEN];
	sfileinfo finfo;

	memmove(finfo.name, filename, FILENMLEN);
	finfo.size = get_filesize(filename);

	if(make_handshake(&finfo) != -1)
	{
		send_batchwise(&finfo);
		printf("Hand shake handeled\n");
	}
}



