#include <stdio.h>
#include <time.h>
#include <stdlib.h>

#include "intr/timer.h"

timer *__t;

timer *init_timer(long offset)
{
	timer *t = (timer *)malloc(sizeof(timer));
	
	clock_gettime(CLOCK_MONOTONIC, &(t->basetime));
	t->offset = offset;
	return t;
}

void reset_timer(timer *t)
{
	clock_gettime(CLOCK_MONOTONIC, &(t->basetime));
}

int timer_reached(timer *t)
{
	long long ref_milisec, cur_milisec;
	struct timespec curtime;
	
	clock_gettime(CLOCK_MONOTONIC, &(curtime));

	
	// 1000 milisecond = 1 sec
	// 1000000 nanaosec = 1 milisec
	// 1000000000 nanosecond = 1 sec

	ref_milisec = (t->basetime.tv_sec)*1000 + ((t->basetime.tv_nsec) / 1000000);
	cur_milisec = (curtime.tv_sec)*1000 + ((curtime.tv_nsec) / 1000000);

	return ((ref_milisec + t->offset) < cur_milisec);
}

void destroy_timer(timer *t)
{
	free(t);
}

void cur_nanosec()
{

	long long cur_milisec;
	struct timespec curtime;
	
	clock_gettime(CLOCK_MONOTONIC, &(curtime));

	printf("time :%ld\n", ((curtime.tv_sec * 1000000000) + curtime.tv_nsec));
}
