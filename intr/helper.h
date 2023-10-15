#ifndef HELPER_H
#define HELPER_H


int isvalid_addr(char *addr);
int isvalid_file(char *filenm);
int get_filesize(char *filenm);
unsigned int bytestonum(unsigned char *arr);
void numtobytes(unsigned char *buffer, unsigned int n);

int isnumber(unsigned char *numstr);
unsigned int tonumber(unsigned char *numstr);

int CEIL(int a, int b);
unsigned char encode_part(unsigned char part);
unsigned char get_partno(unsigned char ch);
unsigned char get_op(unsigned char ch);
#endif
