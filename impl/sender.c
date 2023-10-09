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
// & make appropiate are you available.
// if receiver is ready to accept the file then notify_receiver function 
// return 1 else 0
// notify msg : <SENDING(1)><filename(FILENMLEN)><filesize(NUMSIZE)>
int notify_receiver(fileinfo *finfo)
{
	char filename[FILENMLEN];
	unsigned char send_buf[BUFLEN], recv_buf[BUFLEN];
	int tryno = 0, filesize, sendmsg_size, recvmsg_size, res = 0;
	timer *t = init_timer(_RECV_WAIT);

	memmove(filename, finfo->name, FILENMLEN);
	filesize = finfo->size;
	
	send_buf[OPIDX] = SENDING;							// operation information will be stored in first byte.
	memmove(&(send_buf[FLIDX]), filename, FILENMLEN);	// coping filename
	
	extract_num_bytes(&(send_buf[FLIDX + FILENMLEN]), filesize); // coping filesize

	// now send buffer have notification message that is to be pass to receiver
	
	sendmsg_size = 1 + FILENMLEN + NUMSIZE;			
	
	while(!(timer_reached(t)) && res == 0)
	{

		send_buffer(send_buf, sendmsg_size);
		WAIT(_MSG_WAIT);
		
		recvmsg_size = receive_inbuffer(recv_buf);
		if(recvmsg_size > 0 && recv_buf[OPIDX] == SENDBATCH)
			res = 1;	// batch verification remain.
		else if(recvmsg_size > 0)
			printf("else %x\n", recv_buf[0]);
		tryno += 1;
	}
	return res;
}

int send_batchwise(fileinfo *finfo)
{

	unsigned char recv_buf[BUFLEN], msg;
    unsigned int batches_sent = 0, batchno = 0, total_batches, req_batch, exp_batchno;
    int res = 0,recvmsg_size;
    timer *t = init_timer(_RECV_WAIT);

	total_batches = CEIL(finfo->size, BATCHSIZE);

	while(!(timer_reached(t)) && res == 0)
	{		
		recvmsg_size = receive_inbuffer(recv_buf);
		if(recvmsg_size > 0 && recv_buf[OPIDX] == SENDBATCH && bytes_to_num(&(recv_buf[1])) == 0)	// batch 0
		{
			reset_timer(t);
			res = 1;
		}
		else if(recvmsg_size > 0)
			printf("exp %d but got %d", batchno, bytes_to_num(&(recv_buf[1])));
	}

	printf("sendig batch wise %d res\n", res);
	while(res == 1 && !(timer_reached(t)) && batches_sent <= total_batches)
	{	
			exp_batchno = send_batch(finfo, batches_sent, batchno);
			

			if(exp_batchno == batchno)
			{
				batches_sent += 1;
				batchno = batches_sent % BATCHESPOSSB;
				reset_timer(t);
			}
			else
			{
				printf("batch mismatched exp batch %d\n",exp_batchno);
				res = 0;
			}
			WAIT(_MSG_WAIT);
	}
	
	
}


// send masg : <DATA(1)><partno(4)><data...>
int send_batch(fileinfo *finfo, unsigned int batches_sent, unsigned int batchno)
{

    unsigned char send_buf[BUFLEN];
    FILE *sendfilefp;
    unsigned int partno = 0, sendmsg_size;
    long batchpos = batches_sent * BATCHSIZE;
	int res = -1;

    sendfilefp = fopen(finfo->name, "rb+");
    if(sendfilefp != NULL)
    {
	printf("in send batch\n");
		fseek(sendfilefp, batchpos, SEEK_SET);
        while(!(feof(sendfilefp)) && partno < PARTSINBATCH)
        {
            sendmsg_size = 0;
            send_buf[OPIDX] = concat_batchnop(batchno, DATA);
            extract_num_bytes(&(send_buf[PRTIDX]), partno);

            sendmsg_size = 1 + NUMSIZE;

            sendmsg_size += fread(&(send_buf[DATAIDX]), 1, PARTSIZE, sendfilefp);

			printf("batch %d part %d len %d\n", batches_sent, partno, sendmsg_size-5);
            send_buffer(send_buf, sendmsg_size);
			partno += 1;
        }

        res = send_missing_parts(sendfilefp, batchpos, batchno);
		fclose(sendfilefp);
    }
    else
        printf("can't open file %s\n", finfo->name);
	printf("%d res in send_batch\n", res);
	return res;
}

int send_missing_parts(FILE *sendfilefp, long batchpos, unsigned int batchno)
{
	int partno, recvmsg_size, sendmsg_size, res = -1, req_batch;
	unsigned char recv_buf[BUFLEN], send_buf[BUFLEN], msg;
	timer *t = init_timer(_RECV_WAIT);
	long partpos;

	while(!(timer_reached(t)) && res == 0 && sendfilefp != NULL)
	{
		recvmsg_size = receive_inbuffer(recv_buf);
		req_batch = bytes_to_num(&(recv_buf[1]));
		if(recvmsg_size > 0 && recv_buf[OPIDX] == SENDBATCH && req_batch == (batchno + 1) % BATCHSIZE)
		{
			res = (batchno + 1) % BATCHSIZE;
			printf("sent %d\n", batchno);
		}
		else if(recvmsg_size > 0 && recv_buf[OPIDX] == RESENDPARTS)
		{
			for(int i = PRTIDX; i < (recvmsg_size - 1); i += 4)
			{
				sendmsg_size = 0;
				send_buf[OPIDX] = concat_batchnop(batchno, DATA);

				for(int p = 0; p < NUMSIZE; p++) 	
					send_buf[PRTIDX+p] = recv_buf[PRTIDX+p];	// insted recalculating just resending requested partno bytes

				sendmsg_size = 1 + NUMSIZE;

				partpos = batchpos + (partno * PARTSIZE);
				fseek(sendfilefp, partpos, SEEK_SET);
				sendmsg_size += fread(&(send_buf[DATAIDX]), 1, PARTSIZE, sendfilefp);
				send_buffer(send_buf, sendmsg_size);
			}
		}
		else if(recvmsg_size > 0)
			printf("failed to recover parts waited\n");
	}
	printf("res %d in send_missing\n", res);
	return res;
}

// handle send file operation
void send_file(char *filename)
{
	int res, filesize;
	unsigned char buf[BUFLEN];
	fileinfo finfo;

	memmove(finfo.name, filename, FILENMLEN);	// coping into finfo
	finfo.size = get_filesize(filename);

	res = notify_receiver(&finfo);
	printf("res %d\n", res);	
	if(res == 1)
		send_batchwise(&finfo);
}
