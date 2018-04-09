/* Copyright (C) 2018 Giuseppe Bilotta <giuseppe.bilotta@gmail.com>
 * Released under the Mozilla Public License version 2.0
 */

/* Debug parsing of an expression with fast_parse_float32 */

#include <stdio.h>
#include <stdlib.h>

#define DEBUG_PARSE_FLOAT

#include "parse-float.h"

int main(int argc, char*argv[])
{
	char *end;
	float ret = 0, stdret = 0;
	if (argc < 2) {
		fprintf(stderr, "please provide a value");
		return -1;
	}

	ret = fast_parse_float32(argv[1], &end);
	stdret = strtof(argv[1], NULL);

	printf("= %.12g\ns %.12g\n", ret, stdret);
	printf("= %#"PRIx32"\ns %#"PRIx32"\n",
		*(int*)&ret, *(int*)&stdret);

	return !!ret;
}

