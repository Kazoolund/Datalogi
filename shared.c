#include <stdio.h>
#include "shared.h"

/* A list of names for the different loadbalancing algorithms. */
const char *algorithm_names[BALANCE__MAX] = {
	"No balancing",
	"Round robin",
	"Weighted least"
};

double
timespec_to_double(struct timespec t)
{
	double time;
	time = t.tv_sec;
	time += (double)t.tv_nsec / 1000000000.0;
	return time;
}
