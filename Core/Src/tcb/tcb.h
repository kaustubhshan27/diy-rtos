/*
 * tcb.h
 *
 *  Created on: May 1, 2024
 *      Author: kaustubh
 */

#include "stdint.h"

#define NUM_OF_THREADS	4
#define STACK_SIZE		100

typedef struct tcb {
	uint32_t *stack_ptr;
	struct tcb *next_tcb;
} tcb_type;
