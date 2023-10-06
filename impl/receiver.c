#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "intr/receiver.h"
#include "intr/helper.h"
#include "intr/ops.h"
#include "intr/const.h"
#include "intr/main.h"
#include "intr/timer.h"


// handles if sender want to send the file
// returns filesize of file to be receive 
// upon err return -1
rfileinfo handle_notification()
{
	unsigned char recv_buf[BUFLEN];
	int recvmsg_size, filesize = -1, sizeidx = 1 + FILENMLEN;
	rfileinfo finfo;

	finfo.size = 0;

	timer *t = init_timer(_RECV_WAIT);
	while(!(timer_reached(t)) && (finfo.size == 0)) 	// either we reached to timer or we received infomation
	{
		recvmsg_size = receive_inbuffer(recv_buf);	
		
		if(recvmsg_size > 0 && recv_buf[OPIDX] == SENDING)
		{
			memmove(finfo.name, &(recv_buf[FLIDX]), FILENMLEN);
			finfo.size = bytes_to_num(&(recv_buf[sizeidx]));
		}
	}
	destroy_timer(t);
	return finfo;
}

int collect_file(rfileinfo *finfo)
{
	int i = 0, recvmsg_size, recved_file_size = 0;
	unsigned int partpos, batches_recv = 0, expt_batches, cur_batch = 0, partno;
   	unsigned char recv_buf[BUFLEN];
	timer *t = init_timer(10);

	expt_batches = CEIL(finfo->size, (PARTSINBATCH * PARTSIZE));
	

	while(batches_recv < expt_batches)
	{
		while(!(timer_reached(t)))
		{
			recvmsg_size = receive_inbuffer(recv_buf);
			if(recvmsg_size > 0 && (DATA == get_op(recv_buf[0])))
			{
				partno = bytes_to_num(&(recv_buf[1]));
				recved_file_size += recvmsg_size - 5;
				reset_timer(t);
			}
		}


	}

	return 0;
}

int ask_tosend(rfileinfo *finfo)
{
	int expt_data = 0, recvmsg_size, tryno = 0;
	unsigned char send_buf[BUFLEN], recv_buf[BUFLEN];
	
	send_buf[OPIDX] = SEND;					 
	while((finfo->size > 0) && (tryno < NO_OF_TRIES) && (expt_data == 0))
	{
		// ask to send file	
		send_buffer(send_buf, 1);	
		WAIT(_MSG_WAIT);

		recvmsg_size = receive_inbuffer(recv_buf);
		if(recvmsg_size > 0 && recv_buf[OPIDX] == DATA)
			expt_data = 1;	
		tryno += 1;
	}
	return expt_data;
}

void receive_file(char *filename)
{
	rfileinfo finfo;
	int n = 0, res, expt_data = 0; 

	finfo = handle_notification();			// return file size of file that will be sent 
											
	res = ask_tosend(&finfo);
		printf("expcted file size %d\n", finfo.size);

   	if(finfo.size > 0 && res == 1)						// if notification was handled properly
		collect_file(&finfo);
	else
		printf("Sender was unable to initiate transfer in %d sec.\n", _RECV_WAIT);
}
