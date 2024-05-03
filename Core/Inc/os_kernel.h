/*
 * os_kernel.h
 *
 *  Created on: May 2, 2024
 *      Author: kaustubh
 */

#ifndef SRC_OS_KERNEL_OS_KERNEL_H_
#define SRC_OS_KERNEL_OS_KERNEL_H_

#include "stdint.h"
#include "stm32f4xx.h"

uint8_t os_kernel_add_threads(void(*task0)(void), void(*task1)(void), void(*task2)(void));
void os_kernel_launch(uint32_t quanta);

#endif /* SRC_OS_KERNEL_OS_KERNEL_H_ */
