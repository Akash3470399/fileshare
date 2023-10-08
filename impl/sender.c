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

int notify_batch_cycle(unsigned int batch_cycleno)
{
	char buffer[5], recv_buf;
	int res = 0;
	timer *t = init_timer(_MSG_WAIT);

	buffer[OPIDX] = SENDING;
	extract_num_bytes(&(buffer[1]), batch_cycleno);


	while(res == 0 && !(timer_reached(t)))
	{
		send_buffer(buffer, 5);	// <SENDING><batch_cycleno> total 5 bytes
		
		WAIT(2);
		if((receive_inbuffer(&recv_buf) > 0) && recv_buf == SEND)
			res = 1;
	}	

	printf("notified %d batch %d\n", batch_cycleno, res);
	return res;
}


int send_batchwise(fileinfo *finfo)
{
	int cur_batch_cycle = 0, total_batch_cycles, batchno = 0;	
	timer *bt = init_timer(200);

	total_batch_cycles = CEIL(finfo->size, BATCH_CYCLE_SIZE);

	while(cur_batch_cycle < total_batch_cycles && !(timer_reached(bt)))
	{
		if(notify_batch_cycle(cur_batch_cycle) == 1)
		{
			printf("Sending %d batch\n", batchno);
			batchno = 0;
			while(batchno <= MAXBATCHNUM && !(timer_reached(bt)))
			{
				if(send_batch(finfo, cur_batch_cycle, batchno) == 1)
				{
					batchno += 1;
					reset_timer(bt);
				}		
			}
		}
	}
}

int batch_recovery(fileinfo *finfo, int cur_batch_cycle, int batchno)
{
	int partpos, partno, res = 0, recvmsg_size, sendmsg_size;
	unsigned char send_buf[BUFLEN], recv_buf[BUFLEN];
	FILE *fp = fopen(finfo->name, "rb+");
	timer *t = init_timer(20);

	if(fp != NULL)
	{
		while(!(timer_reached(t)) && res == 0)
		{
			recvmsg_size = receive_inbuffer(recv_buf);
			if(recvmsg_size > 0 && get_op(recv_buf[OPIDX]) == RECEIVED)		// everything is fine
				res = 1;													// batch has been sent successfully
			else if(recvmsg_size > 0 && get_op(recv_buf[OPIDX]) == RESEND)
			{	// resending missing parts in batch
				for(int i = DATAIDX; i < recvmsg_size; i += NUMSIZE)
				{
					partno = bytes_to_num(&(recv_buf[i]));
					partpos = (cur_batch_cycle * BATCH_CYCLE_SIZE) + (batchno * BATCHSIZE) + (partno * PARTSIZE);
					fseek(fp, partpos, SEEK_SET);

					send_buf[OPIDX] = concat_batchnop(batchno, DATA);
					extract_num_bytes(&(send_buf[PRTIDX]), partno);
				
					sendmsg_size = 1 + NUMSIZE;
					sendmsg_size += fread(&(send_buf[DATAIDX]), 1, PARTSIZE, fp);

					send_buffer(send_buf, sendmsg_size);
				}
				reset_timer(t);
			}
		}
		fclose(fp);	
	}
	return res;
}



// 
int send_batch(fileinfo *finfo, int cur_batch_cycle, int batchno)
{
	FILE *fp;
	unsigned char send_buf[BUFLEN];
	int sendmsg_size;
	unsigned int partno, res = 0, last_batch_cycle, batchpos, parts_in_batch;
   
	last_batch_cycle = CEIL(finfo->size, BATCH_CYCLE_SIZE);

	fp = fopen(finfo->name, "rb+");
	parts_in_batch = (cur_batch_cycle == last_batch_cycle)? last_batch_cycle % PARTSINBATCH : PARTSINBATCH;


	if(fp != NULL)
	{
		batchpos = (BATCH_CYCLE_SIZE * cur_batch_cycle) + (batchno * BATCHSIZE);
		fseek(fp, batchpos, SEEK_SET);

		while(!feof(fp) && partno < parts_in_batch)
		{
			send_buf[OPIDX] = concat_batchnop(batchno, DATA);
			extract_num_bytes(&(send_buf[PRTIDX]), partno);

			sendmsg_size = 1 + NUMSIZE; 

			sendmsg_size += fread(&(send_buf[DATAIDX]), 1, PARTSIZE, fp);	

			send_buffer(send_buf, sendmsg_size);
			partno += 1;
		}

		fclose(fp);
		res = batch_recovery(finfo, cur_batch_cycle, batchno);		// handle recovery if any
		printf("cycle %d batch %d, res %d\n", cur_batch_cycle, batchno, res);
	}
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
	
	if(res == 1)
		send_batchwise(&finfo);
}
