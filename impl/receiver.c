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

// handles if sender want to send the file
// returns filesize of file to be receive 
// upon err return -1
rfileinfo handle_notification()
{
	unsigned char recv_buf[BUFLEN];
	int recvmsg_size, filesize = -1, sizeidx = 1 + FILENMLEN;
	rfileinfo finfo;
	char path[FILENMLEN+FILENMLEN];


	finfo.size = 0;

	timer *t = init_timer(_RECV_WAIT);
	while(!(timer_reached(t)) && (finfo.size == 0)) 	// either we reached to timer or we received infomation
	{
		recvmsg_size = receive_inbuffer(recv_buf);	

		if(recvmsg_size > 0 && recv_buf[OPIDX] == SENDING)
		{
			//finfo.name[0] = 'r', finfo.name[1] = '/';
			//memmove(&(finfo.name[2]), &(recv_buf[FLIDX]), FILENMLEN);
			finfo.size = bytes_to_num(&(recv_buf[sizeidx]));
		}
	}
	destroy_timer(t);
	return finfo;
}

int recover_missing_parts(rfileinfo *finfo, unsigned int batchpos, unsigned int batchno)
{
	unsigned char send_buf[BUFLEN], recv_buf[BUFLEN];
	int arr[PARTSINBATCH], res = 0;
	int misspartcnt = getmisprts(finfo->pd, arr);
	unsigned int sendmsg_size, recvmsg_size;

	timer *t = init_timer(_RECV_WAIT);
	printf("missed %d\n", misspartcnt);
	
	send_buf[OPIDX] = SENDBATCH;
	extract_num_bytes(&(send_buf[PRTIDX]), (batchno+1));
	sendmsg_size = 5;

	while(!(timer_reached(t)) && res == 0)
	{
		send_buffer(send_buf, sendmsg_size);
		WAIT(_MSG_WAIT);
		recvmsg_size = receive_inbuffer(recv_buf);
		if(recvmsg_size > 0 && get_op(recv_buf[0]) == DATA)
		{	res = 1;

			printf("received %d batch, %d part", get_batchno(recv_buf[0]), bytes_to_num(&(recv_buf[1])));
		}
		else if(recvmsg_size > 0)
			printf("else %x", recv_buf[0]);
	}

	return res;
}

int receive_batch(rfileinfo *finfo, unsigned int batches_recv, unsigned int batchno)
{
	unsigned char recv_buf[BUFLEN];
	unsigned int recvmsg_size, recvfile_size, partno = 0;
	timer *t = init_timer(_MSG_WAIT);
	int res = 1;
	unsigned int batchpos = batches_recv * BATCHSIZE, totalbatches, expd_parts, partpos;
	FILE *recvfp;

	totalbatches = CEIL(finfo->size, BATCHSIZE);
	expd_parts = ((batches_recv+1) == totalbatches)? PARTSINBATCH: PARTSINBATCH;	

	batchpos = batches_recv * BATCHSIZE;
	int fd = open(finfo->name, O_CREAT | O_RDWR, 0777);	
	recvfp = fopen(finfo->name, "wb+");
	if(fd < 0) printf("open failed..");
	if(recvfp != NULL)
	{
		printf("filename %s \n", finfo->name);
		finfo->pd = init_prtdata(expd_parts); 
		while(!(timer_reached(t)) && partno < expd_parts)
		{
			recvmsg_size = receive_inbuffer(recv_buf);
			if(recvmsg_size > 0 && get_op(recv_buf[OPIDX]) == DATA && get_batchno(recv_buf[OPIDX]) == batchno)
			{
				partno = bytes_to_num(&(recv_buf[PRTIDX]));
				set(finfo->pd, partno);
				printf("batch %d, part %d, len %d\n", batchno, partno, recvmsg_size-5);
				partpos = batchpos + (partno*PARTSIZE);
					
				//int e = fseek(recvfp, partpos, SEEK_SET);
				//int w = fwrite(&(recv_buf[DATAIDX]), 1, recvmsg_size - 5, recvfp);
				fflush(recvfp);
				int e = lseek(fd, partpos, SEEK_SET);
				printf("seek : %d", e);
				int w = write(fd, &(recv_buf[DATAIDX]), recvmsg_size-5);
				printf("written %d msg", w);
				reset_timer(t);
			}
			else if(recvmsg_size > 0)
				printf("batchno missmatch exp %d got %d\n", batchno, get_batchno(recv_buf[OPIDX]));
		}
		close(fd);
		fclose(recvfp);
		if(recover_missing_parts(finfo, batchpos, batchno))
			res = 1;
		else
			printf("recved miss failed\n");
		destroy_prddata(finfo->pd);
	}
	else
	{
		printf("cant store at this moment at %s\n", finfo->name);
	}
	return res;
}


int receive_batchwise(rfileinfo *finfo)
{
	unsigned char recv_buf[BUFLEN], send_buf[BUFLEN];
	unsigned int recvmsg_size = 0, sendmsg_size, batces_recv = 0, batchno = 0;
	unsigned int total_batches = CEIL(finfo->size, BATCHSIZE), batches_poss;
	long recvfile_size = 0;
	int res = 0, stop = 0;
	timer *t = init_timer(_RECV_WAIT);

	// asking to send next batch
		send_buf[OPIDX] = SENDBATCH;
		extract_num_bytes(&(send_buf[PRTIDX]), batchno);
		sendmsg_size = 5;

		batchno = batces_recv % BATCHESPOSSB;
		while(!(timer_reached(t)) && res == 0)
		{
			send_buffer(send_buf, sendmsg_size);
			WAIT(_MSG_WAIT);
			recvmsg_size = receive_inbuffer(recv_buf);
			if(recvmsg_size > 0 && get_op(recv_buf[0]) == DATA)
				res = 1;
			else if(recvmsg_size > 0)
				printf("else %x", recv_buf[0]);
		}

		printf("recv batchwise res %d", res);

	while(res == 1 && !(timer_reached(t)) && batces_recv <= total_batches)
	{
				printf("collecting %d batch\n", get_batchno(recv_buf[0]));
		batches_poss = (batces_recv == total_batches)? finfo->size % BATCHSIZE : BATCHESPOSSB;

		while(batchno < batches_poss && !(timer_reached(t)) && stop ==  0)
		{
			if(receive_batch(finfo, batces_recv, batchno))
			{
				batces_recv += 1;
				batchno += 1;
				if(batces_recv % BATCHESPOSSB == 0)
					stop = 1;
				WAIT(_MSG_WAIT);
			}
		}
	}
		return 0;
}

void receive_file(char *filename)
{
	rfileinfo finfo;
	int n = 0, res, expt_data = 0; 

	finfo = handle_notification();			// return file size of file that will be sent 

	//res = ask_tosend(&finfo);
	printf("expcted file size %d\n", finfo.size);

	strcpy(finfo.name, "out");

	if(finfo.size > 0)						// if notification was handled properly
		receive_batchwise(&finfo);
	else
		printf("Sender was unable to initiate transfer in %d sec.\n", _RECV_WAIT);
}
