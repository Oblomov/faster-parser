/* Copyright (C) 2018 Giuseppe Bilotta <giuseppe.bilotta@gmail.com>
 * Released under the Mozilla Public License version 2.0
 */

/* Script to benchmark parsing of a large variety of floating-point
 * values, comparing the performance of fast_parse_float32
 * with the performance of the standard library.
 */

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if defined(__AVX2__)
#include "simd-int.h"
#else
#include "parse-int.h"
#endif

size_t fill_buffer(char *buffer, size_t size, int64_t start, int64_t end)
{
	size_t pos = 0;
	memset(buffer, 0, size);

	for (int64_t i = start; i < end; ++i)
		pos += snprintf(buffer + pos, size - pos, "%"PRId64"/%"PRId64"/%"PRId64"\n", i-1, i, i+1);
	return pos;
}

#if defined(__AVX2__)
/* Parse the buffer using fast_parse_int64, returns the number of parsed floats
 */
size_t simd_parse_buffer(const char *buffer, int64_t *results, size_t max_ints)
{
	const char *from = NULL;
	char *end = (char*)buffer;
	size_t i;
	for (i = 0; i < max_ints && end != from; ++i) {
		from = end;
		if (*from == '/') ++from;
		/* TODO SIMD skip */
		else while (isspace(*from)) ++from;
		results[i] = simd_parse_int64(from, &end);
	}
	return i;
}
#endif

/* Parse the buffer using fast_parse_int64, returns the number of parsed floats
 */
size_t fast_parse_buffer(const char *buffer, int64_t *results, size_t max_ints)
{
	const char *from = NULL;
	char *end = (char*)buffer;
	size_t i;
	for (i = 0; i < max_ints && end != from; ++i) {
		from = end;
		if (*from == '/') ++from;
		else while (isspace(*from)) ++from;
		results[i] = fast_parse_int64(from, &end);
	}
	return i;
}

/* Parse the buffer using strtof, returns the number of parsed floats
 */
size_t std_parse_buffer(const char *buffer, int64_t *results, size_t max_ints)
{
	const char *from = NULL;
	char *end = (char*)buffer;
	size_t i;
	for (i = 0; i < max_ints && end != from; ++i) {
		from = end;
		if (*from == '/') ++from;
		results[i] = strtol(from, &end, 10);
	}
	return i;
}

void report_performance(clock_t tics, const char *op, size_t size, const char *expr)
{
	double runtime = tics;
	runtime /= CLOCKS_PER_SEC;
	printf("%s: %zu in %-6.4gsec, %-6.4gM%s\n", op, size, runtime, size/runtime/1.0e6, expr);
	fflush(stdout);
}

void compare_results(
	size_t num_simd, const int64_t * restrict simd,
	size_t num_fast, const int64_t * restrict fast,
	size_t num_std, const int64_t * restrict std)
{
	size_t count = num_fast;
	if (num_fast > num_std)
		count = num_std;
	if (num_fast != num_std)
		fprintf(stderr, "fast/std count mismatch, using %zu of %zu/%zu\n", count, num_fast, num_std);
	for (size_t i = 0; i < count; ++i) {
		if (fast[i] != std[i])
			fprintf(stderr, "misparsed %zu: %"PRId64" vs %"PRId64"\n", i, fast[i], std[i]);
	}
	if (simd) {
		if (num_simd < num_std)
			count = num_simd;
		if (num_simd != num_std)
			fprintf(stderr, "simd/std count mismatch, using %zu of %zu/%zu\n", count, num_simd, num_std);
		for (size_t i = 0; i < count; ++i) {
			if (simd[i] != std[i])
				fprintf(stderr, "misparsed %zu: %"PRId64" vs %"PRId64"\n", i, simd[i], std[i]);
		}
	}
}

int main()
{
#define BUFSIZE UINT32_C(1024*1024*1024)
#define NUMINTS INT64_C(1024*1024*16)
#define NUMRESULTS (3*NUMINTS)

	char *buffer = malloc(BUFSIZE);
	int64_t *simdresults = calloc(NUMRESULTS, sizeof(int64_t));
	int64_t *fastresults = calloc(NUMRESULTS, sizeof(int64_t));
	int64_t *stdresults = calloc(NUMRESULTS, sizeof(int64_t));

	if (!buffer) {
		fprintf(stderr, "failed to allocate buffer\n");
		exit(-1);
	}
	if (!fastresults || !stdresults) {
		fprintf(stderr, "failed to allocate results\n");
		exit(-1);
	}

	size_t simd_parsed, fast_parsed, std_parsed, filled_bytes;

	clock_t tic, toc;

	const int64_t starts[] = {
		-NUMINTS*NUMINTS, -NUMINTS*NUMINTS/1024, -NUMINTS*NUMINTS/(1024*1024), -NUMINTS,
		0,
		NUMINTS, NUMINTS*NUMINTS/(1024*1024), NUMINTS*NUMINTS/1024, NUMINTS*NUMINTS,
		INT64_MAX };

	for (const int64_t *start = starts; *start != INT64_MAX; ++start) {
		printf("====> Starting from %"PRId64"\n", *start);
		fflush(stdout);

		tic = clock();
		filled_bytes = fill_buffer(buffer, BUFSIZE, *start, *start + NUMINTS);
		toc = clock();
		report_performance(toc - tic, "Clear and fill buffer", BUFSIZE + filled_bytes, "B/sec");

#if defined(__AVX2__)
		tic = clock();
		simd_parsed = simd_parse_buffer(buffer, simdresults, NUMRESULTS);
		toc = clock();
		report_performance(toc - tic, "SIMD parse", simd_parsed, "int64/sec");
#endif

		tic = clock();
		fast_parsed = fast_parse_buffer(buffer, fastresults, NUMRESULTS);
		toc = clock();
		report_performance(toc - tic, "Fast parse", fast_parsed, "int64/sec");

		tic = clock();
		std_parsed = std_parse_buffer(buffer, stdresults, NUMRESULTS);
		toc = clock();
		report_performance(toc - tic, "Std parse ", std_parsed, "int64/sec");

		tic = clock();
		compare_results(
			simd_parsed, simdresults,
			fast_parsed, fastresults,
			std_parsed, stdresults);
		toc = clock();
		report_performance(toc - tic, "Comparison", std_parsed, "int64/sec");
	}

	return 0;
}
