#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>

#include "intr/globals.h"
#include "intr/helper.h"
#include "intr/consts.h"
#include "intr/receiver.h"
#include "intr/sender.h"

unsigned int sendmsglen;
unsigned int recvmsglen;

unsigned char *sendbuf;
unsigned char *recvbuf;

unsigned char op;

int sockfd;
struct sockaddr_in receiver_addr, self_addr;


int config_socket(char *r_addr)
{
	struct in_addr addr;

	memset(&receiver_addr, 0, sizeof(receiver_addr));
	memset(&self_addr, 0, sizeof(self_addr));

	if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		printf("Socket creation failed\n");
		exit(EXIT_FAILURE);
	}

	if(inet_aton(r_addr, &addr) == 0)
	{
		printf("Invalid receiver address\n");
		close(sockfd);
		exit(EXIT_FAILURE);
	}

	// initilizing address 
	receiver_addr.sin_family = AF_INET;
	receiver_addr.sin_port = htons(recvport);
	receiver_addr.sin_addr = addr;

	self_addr.sin_family = AF_INET;
	self_addr.sin_port = htons(selfport);

	if(bind(sockfd, (struct sockaddr *)&self_addr, (socklen_t)sizeof(self_addr)) < 0)
	{
		printf("Bind failed\n");
		close(sockfd);
		exit(EXIT_FAILURE);
	}
}

// send the msg on preconfigured socket
int send_msg(unsigned char *msg, int msglen)
{
	int sentbytes;

	sentbytes  = sendto(sockfd, msg, msglen, MSG_DONTWAIT, (struct sockaddr *)&(receiver_addr), (socklen_t)sizeof(receiver_addr));

	if(sentbytes == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
		sentbytes = 0;
	else if(sentbytes == -1)
	{
		printf("send_msg: %s", strerror(errno));
		sentbytes = 0;
	}

	return sentbytes;
}

// if any msg is receied on socket then stores it on msg
int recv_msg(unsigned char *msg)
{
	int recvedbytes;

	recvedbytes = recvfrom(sockfd, msg, buflen, MSG_DONTWAIT, NULL, NULL);

	if(recvedbytes == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
		recvedbytes = 0;
	else if(recvedbytes == -1)
	{
		printf("recv_msg: %s", strerror(errno));
		recvedbytes = 0;
	}

	return recvedbytes;
}

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
		memmove(str, &(cmd[fileidx]), FILENMLEN);
		while((tryno < 3) && (res == 0))
        {
		    if(isvalid_file(str) == 1)
			{	
				res = 1;
				memmove(&(cmd[fileidx]), str, FILENMLEN);
			}
			else
			{
				printf("Seems like you have enter wrong filename to send\n");
				printf("Enter file to send:");
				scanf("%s", str);
			}
			tryno += 1;
		}
	}
	return res;
}

int main(int c, char* argv[])
{
	char addr[16] = "127.0.0.1";
	char cmd[32];
	
	//printf("Enter address :");
	//scanf("%s\n", addr);

	init_globals(atoi(argv[1]), atoi(argv[2]));
	
	// all socket & address setup
	config_socket(addr);

	// buffers to be use for sending & receiveing
	sendbuf = (char *)malloc(sizeof(buflen));
	recvbuf = (char *)malloc(sizeof(buflen));

	// accepting command
	fgets(cmd, sizeof(cmd), stdin);
	cmd[strlen(cmd)-1] = '\0';

	if(isvalid_send_cmd(cmd) == 1)
		send_file(&cmd[5]);	
	else if(strcmp(cmd, "receive") == 0)
		receive_file();
	else if(!strcmp(cmd, "exit") == 0)
		printf("Invalid command\n");
	return 0;
}
