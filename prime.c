#include "prime.h"

/*
 * int is_prime(int lower, int upper) (FOR DEMO PURPOSES)
 * Takes two inputs: int lower and int upper.
 * int lower represents the lower limits of the numbers to be checked
 * int upper represents the upper limits of the numbers to be checked
 * 
 * When all numbers from lower to upper has been checked, an int value of the 
 * counted primes is returned.
 */
int is_prime(int lower, int upper) {
	int count = 0;
	int number_to_check;

	for (number_to_check = lower; number_to_check <= upper; number_to_check++)
	{	
		int division;
		int isprime = 1;
		for (division = 1; division < number_to_check; division++)
		{	
			double divided = (number_to_check / (double)division); /* One of the integars has to be considered a double to return a double */
			double fraction = fmod(divided, 1);

			if (division != 1 && division != number_to_check && fraction == 0.00000000)
				isprime = 0;
		}

		if (isprime)
			count++;
	}
	
	return count;
}

/*
 * int weighted_prime_worker(int weight, int lower, int upper)
 * Takes 3 inputs:
 * int weight represent the worker-nodes weight (How quick it is), higher is better. (Max 100)
 * int lower represents the lower limits of the numbers to be checked
 * int upper represents the upper limits of the numbers to be checked
 * 
 * Returns the prime count from the lower to upper limit.
 */
int weighted_prime_worker(int weight, int lower, int upper) {
	clock_t start_time = clock();
	int prime_count = is_prime(lower, upper);
	clock_t end_time = clock();
	double time_elapsed = (double)(end_time - start_time) / CLOCKS_PER_SEC;

	if (weight < MAX_WEIGHT) {
		int weight_difference = MAX_WEIGHT - weight;
		double time_multiplier = time_elapsed * weight_difference;
		double new_time = time_multiplier - time_elapsed;

		if (new_time > 1)
			sleep_ms(new_time * MILLISECOND_TO_SECOND);

	}
	return prime_count;
}

/* Custom sleep function for more varity
 * void sleep_ms(double msec)
 * Takes 1 input:
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