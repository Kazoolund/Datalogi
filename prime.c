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
	clock_t start_time = clock();
	result_t prime_count = is_prime(new_task.from, new_task.to);
	clock_t end_time = clock();
	double time_elapsed = (double)(end_time - start_time) / CLOCKS_PER_SEC;

	if (weight < MAX_WEIGHT) {
		double new_time = (MAX_WEIGHT/weight) * time_elapsed;
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
	clock_t start_time = clock();
	double time_elapsed = 0.00;

	do
	{
		clock_t current_clock = clock();
		time_elapsed = (double)(current_clock - start_time) / CLOCKS_PER_SEC * MILLISECOND_TO_SECOND;
	} while ( time_elapsed < msec);

}
