#include <stdio.h>

#include "intr/helper.h"
#include "intr/ops.h"

int main()
{

	unsigned short int a = 31;
	unsigned char op = 7;

	op = concat_batchnop(a, op);
	printf("combined :%x\n", op);

	printf("batchno :%x\n", get_batchno(op));

	printf("op: %x\n", get_op(op));
}
