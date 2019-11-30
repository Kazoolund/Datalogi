#include "shared.h"

/* A list of names for the different loadbalancing algorithms. */
const char *algorithm_names[BALANCE__MAX] = {
	"No balancing",
	"Round robin",
	"Weighted least"
};
