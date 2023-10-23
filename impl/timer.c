#include <stdio.h>
#include <time.h>
#include <stdlib.h>

#include "intr/timer.h"

timer *__t;
long long MSGMIN = -1, MSGMAX = -1, MSGLAST = -1;


// offset in miliseconds
timer *init_timer(long offset)
{
	timer *t = (timer *)malloc(sizeof(timer));
	
	clock_gettime(CLOCK_MONOTONIC, &(t->basetime));
	t->offset = offset;
	return t;
}

void reset_timer(timer *t)
{
	//clock_gettime(CLOCK_MONOTONIC, &(t->basetime));
	t->offset += t->offset;
}

void reinit_timer(timer *t, long offset)
{
	clock_gettime(CLOCK_MONOTONIC, &(t->basetime));
	t->offset = offset;
}


// assuming timer is reached & user want to add new offset to same timer
void reset_timer_offset(timer *t, long offset)
{
	//clock_gettime(CLOCK_MONOTONIC, &(t->basetime));
	t->offset += offset;
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

	printf("time :%lld\n", ((curtime.tv_sec * 1000000000) + curtime.tv_nsec));
}

// mark min, max & last time accordingly
void record_time(timer *t)
{
	long long cur_milisec, ref_milisec, newtime;
	struct timespec curtime;

	clock_gettime(CLOCK_MONOTONIC, &(curtime));

	ref_milisec = (t->basetime.tv_sec)*1000 + ((t->basetime.tv_nsec) / 1000000);
	cur_milisec = (curtime.tv_sec)*1000 + ((curtime.tv_nsec) / 1000000);

	newtime = cur_milisec - ref_milisec;
	printf("cur %lld, ref %lld\n", cur_milisec, ref_milisec);
	if(newtime <= 0) newtime = 1;

	if(MSGLAST == -1)
	{
		MSGMIN = newtime;
		MSGMAX = newtime;
	}
	else
	{
		if(newtime < MSGMIN)
			MSGMIN = newtime;

		if(newtime > MSGMAX)
			MSGMAX = newtime;
	}
	MSGLAST = newtime;
}

void print_reftime()
{
	printf("MSGMIN %lld\n", MSGMIN);
	printf("MSGMAX %lld\n", MSGMAX);
	printf("MSGLAST %lld\n", MSGLAST);
}

