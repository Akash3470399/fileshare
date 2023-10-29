#include <stdio.h>
#include <stdlib.h>

#include "intr/part.h"
#include "intr/helper.h"

#define GRPTYPE unsigned int
#define GRPSIZE (sizeof(GRPTYPE)*8)


/*
 *	This file implements the function for storing part data in bitmap format
 *	Here a array of unsigned int is used to store information about a part is present or nor
 *	as we know an unsinged int have 32 bits so setting appropiate bit we can store information
 *	about 32 parts in one int.
 *	
 *	to set d
 *
 */
	
// creating new prtdata struct
struct prtdata *init_prtdata(int size)
{
	struct prtdata *pd;
	int nprt;
	pd = (struct prtdata *)malloc(sizeof(struct prtdata));

	nprt = CEIL(size, (sizeof(GRPTYPE)*8));
	pd->n = nprt;
	pd->mem = (GRPTYPE *)malloc(sizeof(GRPTYPE) * nprt);
	
	pd->expcnt = size;
	for(int i = 0; i < pd->n; i++)
		pd->mem[i] = 0;
	return pd;
}

void destroy_prddata(struct prtdata *pd)
{
	free(pd->mem);
	free(pd);
}
// mark as pos'th element as present
void set(struct prtdata *pd, unsigned int pos)
{
	unsigned int grpno, grppos;
  	
	if(pd != NULL)
	{
		grpno = pos/(GRPSIZE);
   		grppos = pos % (GRPSIZE);

		pd->mem[grpno] |= 1<<grppos;
	}
}

// remove pos'th element
void reset(struct prtdata *pd, unsigned int pos)
{
	unsigned int grpno, grppos;
  	if(pd != NULL)
	{
		grpno = pos/(GRPSIZE);
   		grppos = pos % (GRPSIZE);
		pd->mem[grpno] &= ~(1<<grppos);
	}
}


int ispart_present(struct prtdata *pd, unsigned int pos)
{
	int resp = -1;
	unsigned int grpno, grppos;
	if(pd != NULL)
	{
		grpno = pos / (GRPSIZE);
		grppos = pos % (GRPSIZE);
		resp = pd->mem[grpno] & (1<<grppos);
	}
	return resp;
}

// get next 
unsigned int getmisprts(struct prtdata *pd, unsigned char *arr)
{
	unsigned int grpno, grppos, i, j, k = 0;
	struct sr s;
	
	s.found = 0;
	i = 0;
	j = 0;

	while(i < pd->n-1)
	{
		if(pd->mem[i] != 0xFFFFFFFF)
		{
			while(j < (GRPSIZE))
			{
				if((pd->mem[i] & 1<<j) == 0)
					arr[k++] = ((GRPSIZE) * i) + j;
				j += 1;
			}
		}
		i += 1;
		j = 0;
	}

	while(j < (pd->expcnt % (GRPSIZE)))
	{
		if((pd->mem[i] & 1<<j) == 0)
			arr[k++] = ((GRPSIZE) * i) + j;
		j += 1;
	}

	return k;
}


void print_values(struct prtdata *pd)
{
	int i;
	for(i = 0; i < pd->n -1; i++)
	{
		for(int j = 0; j < (GRPSIZE); j++)
		{
			if((pd->mem[i] & 1<<j) == (1<<j))
				printf("%ld ", ((i*(GRPSIZE))+j));
		}
	}
	
	int last  = ((pd->expcnt % GRPSIZE) == 0)? GRPSIZE : (pd->expcnt % GRPSIZE);
	for(int j = 0; j <last; j++)
		if((pd->mem[i] & 1<<j) == (1<<j))
			printf("%ld ", ((i*(GRPSIZE))+j));

	printf("\n");
}
