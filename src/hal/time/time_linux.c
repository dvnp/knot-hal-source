/*
 * Copyright (c) 2016, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */

#include <unistd.h>
#include <inttypes.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>

#include "include/time.h"

#define MS 1
#define US 0

static uint32_t get_time(uint8_t unit)
{
	struct timespec spec;
	uint32_t t;
	long mult, div;

	clock_gettime(CLOCK_REALTIME, &spec);

	if (unit) {
		mult = 1000;
		div = 1.0e6;
	} else {
		mult = 1.0e6;
		div = 1.0e3;
	}

	t = (uint32_t)(spec.tv_sec * mult);
	t += (uint32_t)(spec.tv_nsec / div);

	return t;
}

uint32_t hal_time_ms(void)
{
	return get_time(MS);
}

uint32_t hal_time_us(void)
{
	return get_time(US);
}

void hal_delay_ms(uint32_t ms)
{
	usleep(ms * 1000);
}

void hal_delay_us(uint32_t us)
{
	usleep(us);
}

int hal_timeout(uint32_t current,  uint32_t start,  uint32_t timeout)
{
	/* Time overflow */
	if (current < start)
		/* Fit time overflow and compute time elapsed */
		current += (ULONG_MAX - start);
	else
		/* Compute time elapsed */
		current -= start;

	/* Timeout is flagged */
	return (current >= timeout);
}

uint32_t hal_random_value(uint32_t interval, uint32_t ntime, uint32_t min)
{
	uint32_t value = 9973 * ~hal_time_us();

	srand((unsigned int)value);

	value = (rand() % interval) * ntime;
	if (value < min)
		value += min;


	return value;
}
