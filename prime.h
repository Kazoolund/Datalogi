#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <inttypes.h>


#define MAX_WEIGHT 100
#define MILLISECOND_TO_SECOND 1000
#define MICROSECOND_TO_MILLISECOND 1000

result_t is_prime(uint32_t from, uint32_t to);
result_t weighted_prime_worker(weight_t weight, struct task new_task);
void sleep_ms(double msec);