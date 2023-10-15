#ifndef PART_H
#define PART_H

struct prtdata
{
	unsigned int *mem;
	int n, expcnt;
};

struct sr
{
	short int found;
	unsigned int val;
};
struct prtdata *init_prtdata(int size);
void set(struct prtdata *pd, unsigned char pos);
int ispart_present(struct prtdata *pd, unsigned char pos);
void reset(struct prtdata *pd, unsigned char pos);
void destroy_prddata(struct prtdata *);
void print_values(struct prtdata *pd);

unsigned int getmisprts(struct prtdata *pd, unsigned char *arr);
#endif
