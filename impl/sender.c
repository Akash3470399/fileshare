#include <stdio.h>
#include <string.h>

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

	memmove(filename, finfo->name, FILENMLEN);
	filesize = finfo->size;
	
	send_buf[OPIDX] = SENDING;							// operation information will be stored in first byte.
	memmove(&(send_buf[FLIDX]), filename, FILENMLEN);	// coping filename
	
	extract_num_bytes(&(send_buf[FLIDX + FILENMLEN]), filesize); // coping filesize

	// now send buffer have notification message that is to be pass to receiver
	
	sendmsg_size = 1 + FILENMLEN + NUMSIZE;			
	
	while(tryno < NO_OF_TRIES && res == 0)
	{
		send_buffer(send_buf, sendmsg_size);
		WAIT(_MSG_WAIT);
		
		recvmsg_size = receive_inbuffer(recv_buf);
		if(recvmsg_size > 0 && recv_buf[OPIDX] == SEND)
			res = 1;

		tryno += 1;
	}
	return res;
}

int send_batchwise(fileinfo *finfo)
{
	int totalbatches, batches_sent = 0, res = 0;
	timer *t = init_timer(5000);

	totalbatches = CEIL(finfo->size, (PARTSINBATCH * PARTSIZE));

	while(!(timer_reached(t)) && (batches_sent < totalbatches))
	{
		res = send_nextbatch(finfo, batches_sent);
		if(res == 1)
		{
			printf("%d batch sent\n", batches_sent);
			batches_sent += 1;
			reset_timer(t);
		}
		WAIT(2);
	}
}

// 
int send_nextbatch(fileinfo *finfo, int batches_sent)
{
	char send_buf[BUFLEN];
	short int batchno, partno = 0;
	unsigned int batch_startpos, sendmsg_size;
	
	batchno = batches_sent % BATCHESPOSSB;
	batch_startpos = batches_sent * (PARTSINBATCH) * (PARTSIZE);
		
	FILE *fp = fopen(finfo->name, "rb+");	

	// part message format
	// <DATA(1)><partno(NUMSIZE)><data....(PARTSIZE)>

	if(fp != NULL)
	{
		fseek(fp, batch_startpos, SEEK_SET);
		while(!feof(fp) && partno < PARTSINBATCH)
		{
			sendmsg_size = 0;
			send_buf[OPIDX] = concat_batchnop(batchno, DATA);	
			extract_num_bytes(&(send_buf[1]), partno);

			sendmsg_size = 1 + NUMSIZE;		
			sendmsg_size += fread(&(send_buf[DATAIDX]), 1, PARTSIZE, fp);
			printf("batchno %d partno %d len %d\n", batchno, partno, sendmsg_size-5);
			send_buffer(send_buf, sendmsg_size);

			WAIT(20);
			partno += 1;	
		}
	}
	return 1;
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
	
	if(res == 1)
		send_batchwise(&finfo);
}
