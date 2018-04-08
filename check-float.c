/* Copyright (C) 2018 Giuseppe Bilotta <giuseppe.bilotta@gmail.com>
 * Released under the Mozilla Public License version 2.0
 */

/* For every non-negative floating-point value, check that the ASCII
 * version is parsed by fast_parse_float32 as it is parsed by strtof
 */

#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parse-float.h"

typedef union i2f {
	uint32_t i;
	float f;
} i2f_t;

#define TEST_ALL_FLOATS(iter, start, end) \
	for (i2f_t iter = { .i = start }; iter.i < end; ++iter.i)

#define BUFSIZE 1024
char buffer[BUFSIZE];

void check_float(i2f_t const iter, int depth)
{
	i2f_t stdret, fastret;
	char	*cursor = NULL,
	*stdend = buffer,
	*fastend = NULL;

#define CASE(digs) \
	case digs: snprintf(buffer, BUFSIZE, \
	   "%." #digs "e %." #digs "f %." #digs "g", \
	   iter.f, iter.f, iter.f); break
	switch (depth) {
		CASE(6);
		CASE(9);
		CASE(12);
		CASE(16);
	}

	while (cursor != stdend) {
		cursor = stdend;
		stdret.f = strtof(cursor, &stdend);
		fastret.f = fast_parse_float32(cursor, &fastend);
		if (fastend != stdend) {
			fprintf(stderr,
				"END mismatch\n%s\n"
				"%"PRIdPTR "\n%"PRIdPTR "\n",
				buffer,
				fastend - buffer,
				stdend - buffer);
		}
		if (memcmp(&stdret, &fastret, sizeof(float))) {
			fprintf(stderr,
				"VALUE mismatch\n%.9g with %d digits\n"
				"%s\n"
				"%s\n"
				"%.9g\n%.9g\n"
				"%#"PRIx32"\n%#"PRIx32"\n",
				iter.f, depth,
				buffer, cursor,
				stdret.f, fastret.f,
				stdret.i, fastret.i);
		}
	}
}


int main(int argc, char *argv[])
{

#if 1
	const i2f_t _upper_limit = {.f = HUGE_VALF };
#else
	const i2f_t _upper_limit = {.i = UINT32_C(64*1024) };
#endif
	const uint32_t test_end = argc > 1 ?
		(uint32_t)atoi(argv[1]) :
		_upper_limit.i;

	TEST_ALL_FLOATS(iter, UINT32_C(0), test_end) {
		check_float(iter, 6);
		check_float(iter, 9);
		check_float(iter, 12);
		check_float(iter, 16);
	}
	return 0;
}
