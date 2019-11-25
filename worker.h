#define MAX_WEIGHT 100
#define MILLISECOND_TO_SECOND 1000
#define MICROSECOND_TO_MILLISECOND 1000
#define MAX_USLEEP 999999

int is_prime(int lower, int upper);
int weighted_prime_worker(int weight, int lower, int upper);
void sleep_ms(double sec);