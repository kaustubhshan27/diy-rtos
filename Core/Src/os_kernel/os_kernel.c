/*
 * os_kernel.c
 *
 *  Created on: May 2, 2024
 *      Author: kaustubh
 */

#include "os_kernel.h"

#define T_BIT_OFFSET	24

#define NUM_OF_THREADS	4
#define STACK_SIZE		100		/* number of 4-byte words */

typedef struct tcb {
	uint32_t *stack_ptr;
	struct tcb *next_tcb;
} tcb_type;

tcb_type tcbs[NUM_OF_THREADS];
tcb_type *current_tcb;
uint32_t thread_stack[NUM_OF_THREADS][STACK_SIZE];

void os_kernel_task_stack_init(uint32_t task_index)
{
	/* Task Stack Memory View - considering STACK_SIZE = 100 4-byte words
	 * [99] = xPSR
	 * [98] = PC
	 * [97] = LR
	 * [96 to 92] = R12, R3, R2, R1, R0 - part of the exception frame, stacking/un-stacking performed by the processor automatically at exception entry/exit
	 * [91 to 84] = R11, R10, R9, R8, R7, R6, R5, R4 - not part of the exception frame, explicit saving to be done at the time of context switch */
	tcbs[task_index].stack_ptr = &thread_stack[task_index][STACK_SIZE - 16];

	/* Set the T-bit (bit 24) of the xPSR register to indicate Thumb Mode */
	thread_stack[task_index][STACK_SIZE - 1] = (1U << T_BIT_OFFSET);
}

uint8_t os_kernel_add_threads(void(*task0)(void), void(*task1)(void), void(*task2)(void))
{
	/* Disable IRQs while the tasks are initialized */
	__disable_irq();

	/* Circular linked list of TCBs */
	tcbs[0].next_tcb = &tcbs[1];
	tcbs[1].next_tcb = &tcbs[2];
	tcbs[2].next_tcb = &tcbs[0];

	/* [STACK_SIZE - 2] points to the PC in the exception stack frame */
	os_kernel_task_stack_init(0);
	thread_stack[0][STACK_SIZE - 2] = (uint32_t)(task0);

	os_kernel_task_stack_init(1);
	thread_stack[1][STACK_SIZE - 2] = (uint32_t)(task1);

	os_kernel_task_stack_init(2);
	thread_stack[2][STACK_SIZE - 2] = (uint32_t)(task2);

	/* Initialize the current task pointer */
	current_tcb = &tcbs[0];

	__enable_irq();

	return 1;
}
