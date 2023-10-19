#include <stdio.h>
#include <string.h>
#include <endian.h>
#include <stdint.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#include "intr/helper.h"
#define _BSD_SOURCE


// check if addr is valid ip address
int isvalid_addr(char *addr)
{
	struct in_addr *inp;
	int res;
	res = inet_aton(addr, inp);

	return (res > 0)? 1: 0;
}

// function to check if filename is a valid file
// and current process has access to it (read & write).
int isvalid_file(char *filenm)
{
	int res;
	res = access(filenm, F_OK | R_OK | W_OK);
	return (res == 0)? 1 : 0;
}

// return the size of file.
// on error return -1
int get_filesize(char *filenm)
{
	FILE *fp;
	int size = -1;

	fp = fopen(filenm, "rb+");
	if(fp != NULL)
	{
		fseek(fp, 0, SEEK_END);
		size = ftell(fp);
		fclose(fp);
	}
	else
		printf("err while caculating file size : %s\nfilename %s\n", strerror(errno), filenm);
	return size;
}
// takes 4 bytes of memery array and return int forming from it.
unsigned int bytestonum(unsigned char *arr)
{
	// setting all bits to 0 so that bits can copy using or operation.
	unsigned int num = 0, n;	
	for(int i = 0; i < 4; i++)
	{
		num = num << 8;		
		num |= arr[i];		// copoing 1byte to lower 8 bits.
	}
	n = (unsigned int)le32toh((uint32_t)num);				// converting back from little endian notation
	return n;
}

// store 4 bytes of integer(num) into buffer array.
void numtobytes(unsigned char *buffer, unsigned int n)
{
	unsigned int num = (unsigned int)htole32((uint32_t)n);		// converting to little endian notation
	for(int i = 0; i < 4; i++)
  	{
		// extract first 8 bits & shift it to lower 8 bits 
		// to copy it in char array 
		buffer[i] = (num & 0xFF000000)>>24;	
		num = num<<8;
  	}
}
// calculate ceil |_x_| value for positive integers a & b
int CEIL(int a, int b)
{
	int res = -1;
	if(a != 0)
	{
		res = a/b;
		res += (a%b == 0)? 0 : 1;
	}
	return res;
}
