#include <stdio.h>
#include <string.h>
#include <endian.h>
#include <stdint.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#include "intr/helper.h"
#include "intr/const.h"
#include "intr/ops.h"
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
// checks if numstr is valid number
// numstr must be terminated by '\0' 
int isnumber(unsigned char *numstr)
{
	int res = 1, i=0;
	while(numstr[i] != '\0' && res == 1)
	{
		if(numstr[i] <= '0' && numstr[i] >= '9')
			res = 0;
		i += 1;
	}
	return res;
}

// check if str is alphanumetic string or not
int isalphanum(char *str)
{
	int i = 0, res = 1;
	while(str[i] != '\0' && res == 1)
	{
		if(str[i] >= '0' && str[i] <= '9')
			res = 1;
		else if(str[i] >= 'a' && str[i] <= 'z')
			res = 1;
		else if(str[i] >= 'A' && str[i] <= 'Z')
			res = 1;
		else 
			res = 0;

		i += 1;
	}

	return res;
}


// convert str integer to corresponding int
// numstr must be terminated by '\0' 
unsigned int tonumber(unsigned char *numstr)
{
	unsigned int res = 0, i = 0;
	if(isnumber(numstr) == 1) 
	{
		while(numstr[i] != '\0')
		{
			res *= 10;
			res += numstr[i] - '0';
			i += 1;
		}
	}
	return res;
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

unsigned char encode_part(unsigned char part)
{
	part |= (1<<7);
	return part;
}


// 1st bit represent if it is data operation 
// if yes then remaining 7 bits represent the partno
unsigned char get_partno(unsigned char ch)
{
	ch = ch << 1;
	ch = ch >> 1;
	return ch;
}

// last 3 bits
// interpreat the operator from bits
unsigned char get_op(unsigned char ch)
{
	unsigned char mask = 1<<7, op;
	if((ch & mask) == mask)	// if msb is set then its data operation
		op = DATA;
	else
		op = (ch & 7);	// as last 3 bits represent the opration code
	return op;
}
