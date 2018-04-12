/* Copyright (C) 2018 Giuseppe Bilotta <giuseppe.bilotta@gmail.com>
 * Released under the Mozilla Public License version 2.0
 */

#include <ctype.h> // isspace
#include <inttypes.h>

#define CHECK_SIGN(sign, from) \
	if (*from == '-') { \
		sign = -1; \
		++from; \
	} else if (*from == '+') { \
		++from; \
	}

#define FAST_DIGIT_LOOP(val, c, from) \
	char c = *from; \
	while (c >= '0' && c <= '9') { \
		val = val*10 + (c - '0'); \
		c = *(++from); \
	}

#define FAST_PARSE(bits) \
static inline \
int##bits##_t fast_parse_int##bits(const char* from, char **end) \
{ \
	int##bits##_t val = 0; \
	int##bits##_t sign = 1; \
\
	CHECK_SIGN(sign, from) \
\
	FAST_DIGIT_LOOP(val, c, from) \
\
	*end = (char*)from; \
\
	return sign*val; \
} \
static inline \
uint##bits##_t fast_parse_uint##bits(const char* from, char **end) \
{ \
	uint##bits##_t val = 0; \
\
	FAST_DIGIT_LOOP(val, c, from) \
\
	*end = (char*)from; \
\
	return val; \
}

FAST_PARSE(32)
FAST_PARSE(64)
