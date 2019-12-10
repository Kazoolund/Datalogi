#include <stdio.h>
#include <time.h>


#define MAX_WEIGHT 100

result_t is_prime(uint32_t from, uint32_t to);
result_t weighted_prime_worker(weight_t weight, struct task new_task);
void sleep(double sec);
