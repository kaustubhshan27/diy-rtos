/*
 * tcb.c
 *
 *  Created on: May 1, 2024
 *      Author: kaustubh
 */

#include "tcb.h"

tcb_type tcbs[NUM_OF_THREADS];
tcb_type *current_tcb;
uint32_t tcbs_stack[NUM_OF_THREADS][STACK_SIZE];
