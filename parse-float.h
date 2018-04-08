/* Copyright (C) 2018 Giuseppe Bilotta <giuseppe.bilotta@gmail.com>
 * Released under the Mozilla Public License version 2.0
 */

#include <ctype.h> // isspace
#include <inttypes.h>
#include <math.h> // ldexp

/* TODO static assertion that `float` corresponds to the 32-bit
 * (single precision) IEEE-754 binary format.
 */

/**! Given a mantissa and base-10 exponent, returns the float
 * that most closely approximates mantissa*10^e10
 */
static inline
float base10_to_base2_float(uint64_t m10, int32_t e10)
{
	/* Trivial cases */
	if (!m10) return 0.0f;
	if (!e10) return (float)m10;

	/* We need to compute m2, e2 such that m2*2^e2 is closest
	 * to m10*10^e10.
	 * Since 10^p = 5^p 2^p, we start with e2 = e10
	 * and then need to work on m2 to fit around m10*5^e10
	 */
	int32_t e2 = e10;

	/* We need to compute m10*5^e10, i.e. multiply (resp. divide)
	 * by 5 |e10| times, handling over/underflow. We achieve this
	 * by keeping the mantissa optimally aligned within the 64 bits
	 * of its data types.
	 * Specifically, since multiplying may need up to 3 additional
	 * more significant bits, we ensure that the 3 most significant
	 * bits of the mantissa are always 0 before multiplication.
	 * Conversely, when dividing, we keep the mantissa in the most
	 * significant bits of its data type.
	 * In both cases the result is achieved by shifting the mantissa
	 * appropriately and fixing up the exponent to match the shift.
	 */

#define U64_HI1  UINT64_C(0x8000000000000000)
#define U64_HI3  UINT64_C(0xe000000000000000)
	if (e10 > 0) while (e10 > 0) {
		/* make sure we have room in the higher bits */
		while (m10 & U64_HI3) {
			m10 >>= 1;
			++e2;
		}
		m10 *= 5;
		--e10;
	} else while (e10 < 0) {
		/* keep as much room as possible in the lower bits */
		while (!(m10 & U64_HI1)) {
			m10 <<= 1;
			--e2;
		}
		m10 /= 5;
		++e10;
	}

	// TODO verify if/when this is needed
#if 0
	/* Fit in the lowest 32 bits */
#define U64_HI32 UINT64_C(0xffffffff00000000)
	while (m10 & U64_HI32) {
		m10 >>= 1;
		++e2;
	}
#endif

	/* Cast the mantissa to float, with default rounding mode,
	 * and then fixup the exponent.
	 */
	return ldexp((float)m10, e2);
}

/**! Fast variant of strtof
 */
static inline
float fast_parse_float32(const char *from, char **end)
{
	uint64_t mantissa = 0;
	int32_t exponent = 0;
	/* sign of the value */
	float sign = 1;
	/* sign of the exponent */
	int32_t expsign = 1;

	/* Number of significant decimal digits in the mantissa */
	int sigdig = 0;
	/* Number of decimal digits after the dot */
	int decdig = 0;
	/* TODO assess the performance hit of checking that the
	 * exponent does not have more than 2 significant digits
	 */

	while (isspace(*from))
		++from;

	if (*from == '-') {
		sign = -1;
		++from;
	} else if (*from == '+') {
		++from;
	}

	char c = *from;
	while (c >= '0' && c <= '9') {
		if (sigdig < 9) {
			uint64_t n = mantissa*10 + (c - '0');
			if (n > mantissa) ++sigdig;
			mantissa = n;
		} else {
			--decdig;
		}
		c = *(++from);
	}
	if (c == '.') {
		c = *(++from);
		while (c >= '0' && c <= '9') {
			if (sigdig < 9) {
				uint64_t n = mantissa*10 + (c - '0');
				if (n > mantissa) ++sigdig;
				mantissa = n;
				++decdig;
			}
			c = *(++from);
		}
	}
	if (c == 'e' || c == 'E') {
		++from;
		if (*from == '-') {
			expsign = -1;
			++from;
		} else if (*from == '+') {
			++from;
		}
		c = *from;
		while (c >= '0' && c <= '9') {
			exponent = exponent*10 + (c - '0');
			c = *(++from);
		}
	}
	/* TODO check if we had parsing errors, we would need
	 * to 'roll back' to the initial value of from,
	 * and return 0.0f;
	 */
	*end = (char*)from;

	exponent = exponent*expsign - decdig;

	return sign*base10_to_base2_float(mantissa, exponent);
}
