#include <stdint.h>

#define LOADBALANCER_PORT 5000

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
