#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>		// for socket(), inet_addr()
#include <sys/socket.h>		// for soocket(), bind()
#include <netinet/in.h>		// for inet_adder(), sockaddr_in
#include <arpa/inet.h>		// for htons(), inet_addr()
#include <errno.h>

#include "intr/sender.h"
#include "intr/receiver.h"
#include "intr/helper.h"
#include "intr/const.h"


int sockfd, addrlen;
struct sockaddr_in receiverAddr, selfAddr, senderAddr;

unsigned char sbuf[BUFLEN];
unsigned int smsglen = 0;

unsigned char rbuf[BUFLEN];
unsigned int rmsglen = 0;



// initial configurations of addresses & socket
// also binding of socket
void net_config(char *addr)
{
	memset(&receiverAddr, 0, sizeof(receiverAddr));
	memset(&selfAddr, 0, sizeof(selfAddr));
	memset(&senderAddr, 0, sizeof(senderAddr));


	receiverAddr.sin_family = AF_INET;
	receiverAddr.sin_port = htons(P1);	//htons(RECEIVER_PORT);
	receiverAddr.sin_addr.s_addr = inet_addr("192.168.43.248");

	selfAddr.sin_family = AF_INET;
	selfAddr.sin_port = htons(P2); 		//htons(RECEIVER_PORT);

	if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		perror("socket creation failed.\n");
		exit(EXIT_FAILURE);
	}
	
	printf("sockfd %d\n", sockfd);
	if(bind(sockfd, (const struct sockaddr *)&selfAddr, sizeof(selfAddr)) < 0)	
	{
		perror("bind call failed\n");
		exit(EXIT_FAILURE);
	}
}

int send_buffer(unsigned char *buffer, int len)
{

	int res;
	res = sendto(sockfd, buffer, len, MSG_DONTWAIT, (struct sockaddr *)&receiverAddr, (socklen_t)sizeof(receiverAddr));
	if(res < 0)
		printf("%s\n", strerror(errno));
	return res;
}

int receive_inbuffer(unsigned char *buffer)
{
	int res;
	res = recvfrom(sockfd, buffer, BUFLEN, MSG_DONTWAIT, (struct sockaddr *)&senderAddr,&addrlen);

	if(res < 0 && errno == EAGAIN)
		res = 0;	
	else if(res < 0 )
		printf("%s\n", strerror(errno));
	return res;
}


int main()
{	
	char cmd[64], addr[16] = "127.0.0.1";
	int valid_addr = 0, tryno = NO_OF_TRIES, fileidx = 5;

	
	net_config(addr);

	while(tryno < NO_OF_TRIES && valid_addr == 0)
	{
		printf("Enter receiver address: ");
		scanf("%s", addr);

		if(isvalid_addr(addr) == 1)
			valid_addr = 1;
		else
		{
			printf("That address is invalid!!!\n");
			printf("address like 192.168.143.234\n\n");
		}

		tryno += 1;
		if(tryno == 3)
		{
			printf("Stopping as you haven't specified correct address.\n");
			exit(EXIT_FAILURE);
		}
	}

	while(1)
	{
		printf(">>>");
		fgets(cmd, sizeof(cmd), stdin);
		cmd[strlen(cmd)-1] = '\0';
		if(isvalid_send_cmd(cmd) == 1)
		{
			printf("%s\n", cmd);
			send_file(&(cmd[fileidx]));

		}
		else if(strcmp(cmd, "receive") == 0)
		   receive_file("abc");
		else if(strcmp(cmd,"exit") == 0)
			exit(EXIT_SUCCESS);
		else 
			printf("%s menu...\n", cmd);	
	}
	return 0;
}
