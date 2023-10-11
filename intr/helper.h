#ifndef SHELPER_H
#define HELPER_H


int isvalid_addr(char *addr);
int isvalid_file(char *filenm);
int get_filesize(char *filenm);
unsigned int bytestonum(unsigned char *arr);
void numtobytes(unsigned char *buffer, unsigned int n);

int isnumber(unsigned char *numstr);
unsigned int tonumber(unsigned char *numstr);

int CEIL(int a, int b);
unsigned char concat_batchnop(unsigned short int batchno, unsigned char op);
unsigned short int get_batchno(unsigned char ch);
unsigned char get_op(unsigned char ch);
#endif
