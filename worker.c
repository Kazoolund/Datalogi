#include <stdio.h>
#include <math.h>

int is_prime(int lower, int upper);
int main(void)
{

	return 0;
}

/*
 * Function int is_prime(int lower, int upper) (FOR DEMO PURPOSES)
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
		for (division = 1.0; division < number_to_check; division++)
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