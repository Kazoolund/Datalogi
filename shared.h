#include <stdint.h>

#define LOADBALANCER_PORT 5000

extern const char *algorithm_names[]; /* defined in shared.c */

typedef uint8_t  weight_t;
typedef uint16_t result_t;

struct task {
	uint32_t from;
	uint32_t to;
	uint16_t task_number;
};

struct result {
	result_t result;
	uint16_t task_number;
};

enum balance_type {
	BALANCE_NONE,
	BALANCE_ROUND,
	BALANCE_WEIGHTED,
	BALANCE__MAX
};
