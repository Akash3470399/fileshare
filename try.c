#include <stdio.h>
#include "intr/timer.h"

int main()
{

	timer *t = init_timer(5000);
	printf("timer started\n");
	while(!(timer_reached(t)));
	printf("timer reached\n");
	reset_timer(t);
	printf("timer reset\n");
	while(!(timer_reached(t)));
	printf("timer end\n");
	return 0;
}


