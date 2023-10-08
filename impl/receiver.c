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



// sender will send <SENDING(1)><batch_cycleno(4)>
int handle_batchcycle_notice(unsigned int batch_cycleno, rfileinfo *finfo)
{
	unsigned char buffer[5], recv_buf[BUFLEN], send_buf = SEND;
	timer *t = init_timer(_MSG_WAIT);
	unsigned int recv_batch_cycleno;
	int res = 0, recvmsg_size;

	while(res == 0 && !(timer_reached(t)))
	{
		if(receive_inbuffer(recv_buf) > 0 && recv_buf[OPIDX] == SENDING)
		{
			if(batch_cycleno == bytes_to_num(&(recv_buf[1])))
			{
				reset_timer(t);
				while(!(timer_reached(t)) && res == 0)
				{		
					send_buffer(&send_buf, 1);
					recvmsg_size = receive_inbuffer(recv_buf);

					if(recvmsg_size > 0 && get_op(recv_buf[OPIDX]) == DATA)
					{
						res = 1;
						printf("\nrecv %d ", bytes_to_num(&recv_buf[1]));
						write(1, &(recv_buf[5]), recvmsg_size-1);
					}
				}
			}
		}
	}
	return 1;
}

int receive_batch(rfileinfo *finfo, unsigned int cur_batch_cycle, unsigned int batchno)
{
	int recvmsg_size;
	unsigned int recvfile_size = 0;
	unsigned char recv_buf[BUFLEN];
	timer *t = init_timer(_MSG_WAIT);

	printf("recv %d batch\n", batchno);
	while(!(timer_reached(t)))
	{
		recvmsg_size = receive_inbuffer(recv_buf);
		if(recvmsg_size > 0 && get_op(recv_buf[OPIDX]) == DATA)
		{	
			recvfile_size += recvfile_size - 5;
			printf("\nrecv %d ", bytes_to_num(&recv_buf[1]));
			write(1, &(recv_buf[5]), recvmsg_size - 5);
		}
	}

	reset_timer(t);
	while(!(timer_reached(t)))
	{
		recv_buf[0] = RECEIVED;
		send_buffer(recv_buf, 1);
	}
	return 1;
}


int collect_file(rfileinfo *finfo)
{
	unsigned char recv_buf[BUFLEN];
	unsigned int recvmsg_size, filesize = 0, total_batch_cycles, batchno = 0, cur_batch_cycle = 0;
	timer *t = init_timer(200);

	while(!(timer_reached(t)))
	{
		if(handle_batchcycle_notice(cur_batch_cycle, finfo))
		{	
			while(batchno <= MAXBATCHNUM && !(timer_reached(t)))
			{
				receive_batch(finfo, cur_batch_cycle, batchno);
				batchno += 1;
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

	//res = ask_tosend(&finfo);
	printf("expcted file size %d, res %d\n", finfo.size, res);

	if(finfo.size > 0)						// if notification was handled properly
		collect_file(&finfo);
	else
		printf("Sender was unable to initiate transfer in %d sec.\n", _RECV_WAIT);
}
