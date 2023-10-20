#include <time.h>

#ifndef TIMER_H
#define TIMER_H


typedef struct timer timer;

extern timer *__t;
extern long long MSGMIN, MSGMAX, MSGLAST;

#define WAIT(a) __t = init_timer(a); \
				while(!timer_reached(__t)); \
				destroy_timer(__t); 

struct timer
{
	struct timespec basetime;
	long offset;
};

timer *init_timer(long offset);

void reset_timer(timer *t);
void reinit_timer(timer *t, long offset);

int timer_reached(timer *t);

void reset_timer_offset(timer *t, long offset);
void destroy_timer(timer *t);

void cur_nanosec();
void record_time(timer *t);
void print_reftime();
#endif
