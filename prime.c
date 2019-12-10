#include <time.h>
#include "shared.h"
#include "prime.h"
/*
 * result_t is_prime(uint32_t from, uint32_t to) (FOR DEMO PURPOSES)
 * Takes two inputs: from and to
 * uint32_t 'from' represents the lower limits of the numbers to be checked
 * uint32_t 'to' represents the upper limits of the numbers to be checked
 * 
 * When all numbers from variable 'from' to variable 'to has been checked, an int value of the 
 * counted primes is returned.
 */
result_t is_prime(uint32_t from, uint32_t to) {
	int count = 0;
	uint32_t number_to_check;

	for (number_to_check = from; number_to_check <= to; number_to_check++)
	{	
		uint32_t division;
		int isprime = 1;
		for (division = 1; division < number_to_check; division++)
		{	
			if (division != 1 && division != number_to_check && number_to_check % division == 0)
				isprime = 0;
		}
		if (isprime && number_to_check != 1){
/*			printf("Prime: %d\n", number_to_check); */
			count++;
	}}
	
	return count;
}

/*
 * result_t weighted_prime_worker(weight_t weight, struct task new_task)
 * Takes 2 inputs: weight and new_task
 * weight_t weight represent the worker-nodes weight (How quick it is), higher is better. (Max 100)
 * struct task new_task represents the task struct containing the lower and upper limit of the task to be calculated.
 * 
 * Returns the prime count from the lower to upper limit.
 */
result_t weighted_prime_worker(weight_t weight, struct task new_task) {
	struct timespec start_time;
	struct timespec end_time;

	clock_gettime(CLOCK_MONOTONIC, &start_time);
	result_t prime_count = is_prime(new_task.from, new_task.to);
	clock_gettime(CLOCK_MONOTONIC, &end_time);
	double time_elapsed = timespec_to_double(end_time) - timespec_to_double(start_time);

	if (weight < MAX_WEIGHT) {
		double new_time = (MAX_WEIGHT/(double)weight) * time_elapsed;
		double sleeptime = new_time - time_elapsed;
		sleep_ms(sleeptime * MILLISECOND_TO_SECOND);

	}
	return prime_count;
}

/* Custom sleep function for more varity
 * void sleep_ms(double msec)
 * Takes 1 input: msec
 * double msec represents the milliseconds that needs to elapse before the program returns
 */
void sleep_ms(double msec) {
	struct timespec start_time;
	struct timespec current_time;

	clock_gettime(CLOCK_MONOTONIC, &start_time);
	double time_elapsed = 0.00;

	do
	{
		clock_gettime(CLOCK_MONOTONIC, &current_time);
		time_elapsed = timespec_to_double(current_time) - timespec_to_double(start_time);
	} while ( time_elapsed < msec);

}
