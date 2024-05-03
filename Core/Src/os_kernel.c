/*
 * os_kernel.c
 *
 *  Created on: May 2, 2024
 *      Author: kaustubh
 */

#include "os_kernel.h"

#define T_BIT_OFFSET	24

#define NUM_OF_THREADS	4
#define STACK_SIZE		100			/* number of 4-byte words */

#define CLOCK_FREQ		16000000	/* Configured clock frequency = 16 MHz */

typedef struct tcb {
	uint32_t *stack_ptr;
	struct tcb *next_tcb;
} tcb_type;

tcb_type tcbs[NUM_OF_THREADS];
tcb_type *current_tcb;
uint32_t thread_stack[NUM_OF_THREADS][STACK_SIZE];

static void os_kernel_task_stack_init(uint32_t task_index)
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

/* Launch the first thread */
static void os_scheduler_launch(void)
{
	/* Load the address of current_tcb into R0, R0 = &current_tcb */
	__asm("LDR		R0, =current_tcb");

	/* Load the contents of the address pointed to by R0 into R2, R2 = &current_tcb->stack_ptr */
	__asm("LDR 		R2, [R0]");

	/* Load the contents of the address pointed to by R2 into SP, SP = current_tcb->stack_ptr */
	__asm("LDR 		SP, [R2]");

	/* Restore the context of the thread in the order in which they are saved - 1. Exception Stack Frame, 2. R4-R11 */
	__asm("POP		{R4-R11}");
	__asm("POP		{R0-R3}");
	__asm("POP		{R12}");

	/* Skip LR */
	__asm("ADD 		SP, SP, #4");

	/* Load LR with the pointer to the thread */
	__asm("POP		{LR}");

	/* Skip xPSR */
	__asm("ADD 		SP, SP, #4");

	/* Enable interrupts */
	__asm("CPSIE	I");

	/* Branch to task 0 */
	__asm("BX 		LR");
}

void os_kernel_launch(uint32_t quanta)
{
    /* Reset SysTick */
    SysTick->CTRL = 0x0;

    /* Clear the current value register */
    SysTick->VAL = 0x0;

    /* Set SysTick (IRQn = 15) to lowest priority (7) */
    NVIC_SetPriority(15, 7);

    /* Set reload counter to quanta milliseconds when SysTick is enabled */
    SysTick->LOAD = ((quanta / 1000) * CLOCK_FREQ) - 1;

    /*SysTick is now configured and ready to be started
     * Bit 0: 1 = Enable SysTick
     * Bit 1: 1 = Enable SysTick interrupt
     * Bit 2: 1 = Select internal clock */
    SysTick->CTRL = (1U << 0) | (1U << 1) | (1U << 2);

    /* Start the scheduler */
    os_scheduler_launch();
}

/* Performing the functionality of a static Round-Robin scheduler */
__attribute__((naked)) void SysTick_Handler(void)
{
	/* Disable interrupts */
	__asm("CPSID	I");

	/* Step 1. Suspend the current task: */

	/* 1a. Save R4-R11 to the current task's stack */
	__asm("PUSH		{R4-R11}");

	/* 1b. Load the address of current_tcb into R0, R0 = &current_tcb */
	__asm("LDR		R0, =current_tcb");

	/* 1c. Load the contents of the address pointed to by R0 into R1 which will be the address of SP, R1 = &current_tcb->stack_ptr */
	__asm("LDR		R1, [R0]");

	/* 1d. Store the contents of SP in the address pointed to by R1, current_tcb->stack_ptr = SP */
	__asm("STR 		SP, [R1]");

	/* Step 2. Choose the next thread: */

	/* 2a. Load the contents of the address pointed to R1+4 into R1, R1 = current_tcb->next_tcb */
	__asm("LDR		R1, [R1, #4]");

	/* 2b. Store the contents of R1 into the address pointed to by R0, current_tcb = current_tcb->next_tcb */
	__asm("STR		R1, [R0]");

	/* 2c. Load the contents of the address pointed to by R1 into SP, SP = next_tcb->stack_pointer */
	__asm("LDR		SP, [R1]");

	/* 2d. Restore R4-R11 from the next task's stack */
	__asm("POP		{R4-R11}");

	/* Enable interrupts */
	__asm("CPSIE	I");

	/* Return from SysTick */
	__asm("BX 		LR");
}
