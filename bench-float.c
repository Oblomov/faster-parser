/* Copyright (C) 2018 Giuseppe Bilotta <giuseppe.bilotta@gmail.com>
 * Released under the Mozilla Public License version 2.0
 */

/* Script to benchmark parsing of a large variety of floating-point
 * values, comparing the performance of fast_parse_float32
 * with the performance of the standard library.
 */

#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "parse-float.h"

typedef union i2f {
	uint32_t i;
	float f;
} i2f_t;

#define TEST_ALL_FLOATS(iter, start, end) \
	for (i2f_t iter = { .i = start }; iter.i < end; ++iter.i)

size_t fill_buffer(char *buffer, size_t size, uint32_t start, uint32_t end, int depth)
{
	size_t pos = 0;
	const char *fmt;
	memset(buffer, 0, size);

#define CASE(digs) case digs: fmt = "%." #digs "e %." #digs "f %." #digs "g\n"; break
	switch(depth) {
		CASE(6);
		CASE(9);
		CASE(12);
		CASE(16);
	default: 
		fprintf(stderr, "unsupported depth %d\n", depth);
		exit(-1);
	}
#undef CASE

	TEST_ALL_FLOATS(iter, start, end) {
		pos += snprintf(buffer + pos, size - pos, fmt, iter.f, iter.f, iter.f);
	}
	return pos;
}

/* Parse the buffer using fast_parse_float32, returns the number of parsed floats
 */
size_t fast_parse_buffer(const char *buffer, float *results, size_t max_floats)
{
	const char *from = NULL;
	char *end = (char*)buffer;
	size_t i;
	for (i = 0; i < max_floats && end != from; ++i) {
		from = end;
		results[i] = fast_parse_float32(from, &end);
	}
	return i;
}

/* Parse the buffer using strtof, returns the number of parsed floats
 */
size_t std_parse_buffer(const char *buffer, float *results, size_t max_floats)
{
	const char *from = NULL;
	char *end = (char*)buffer;
	size_t i;
	for (i = 0; i < max_floats && end != from; ++i) {
		from = end;
		results[i] = strtof(from, &end);
	}
	return i;
}

void report_performance(clock_t tics, const char *op, size_t size, const char *expr)
{
	double runtime = tics;
	runtime /= CLOCKS_PER_SEC;
	printf("%s: %zu in %gsec, %gM%s\n", op, size, runtime, size/runtime/1.0e6, expr);
	fflush(stdout);
}

int main()
{
#define BUFSIZE UINT32_C(1024*1024*1024)
#define NUMFLOATS UINT32_C(1024*1024*5)
#define NUMRESULTS (3U*NUMFLOATS)

	char *buffer = malloc(BUFSIZE);
	float *results = calloc(NUMRESULTS, sizeof(float));

	if (!buffer) {
		fprintf(stderr, "failed to allocate buffer\n");
		exit(-1);
	}
	if (!results) {
		fprintf(stderr, "failed to allocate results\n");
		exit(-1);
	}

	size_t fast_parsed, std_parsed, filled_bytes;

	clock_t tic, toc;

	const int depths[] = { 6, 9, 12, 16, 0};
	const uint32_t starts[] = { 0, NUMFLOATS, NUMFLOATS*10,  NUMFLOATS*100, NUMFLOATS*1000, UINT32_MAX };

	for (const uint32_t *start = starts; *start != UINT32_MAX; ++start) {
		printf("====> Starting from %#"PRIx32"\n", *start);
		for (const int *depth = depths; *depth; ++depth) {
			printf("==> Depth:  %d\n", *depth);
			fflush(stdout);

			tic = clock();
			filled_bytes = fill_buffer(buffer, BUFSIZE, *start, *start + NUMFLOATS, *depth);
			toc = clock();
			report_performance(toc - tic, "Clear and fill buffer", BUFSIZE + filled_bytes, "B/sec");

			tic = clock();
			fast_parsed = fast_parse_buffer(buffer, results, NUMRESULTS);
			toc = clock();
			report_performance(toc - tic, "Fast parse", fast_parsed, "floats/sec");

			tic = clock();
			std_parsed = std_parse_buffer(buffer, results, NUMRESULTS);
			toc = clock();
			report_performance(toc - tic, "Std parse", std_parsed, "floats/sec");
		}
	}

	return 0;
}
