#include <immintrin.h>
#include <inttypes.h>
#include <stdio.h>

#include "parse-int.h"

#define DEBUG_SIMD 1
#define LANE8(v, lane) _mm256_extract_epi32(v, lane)
#define EXPLODE_SIMD(v) \
	LANE8(v, 0), LANE8(v, 1), LANE8(v, 2), LANE8(v, 3), \
	LANE8(v, 4), LANE8(v, 5), LANE8(v, 6), LANE8(v, 7)

const char *buffer[] = {
	"/12345678901",
	"1/1234567890",
	"12/123456789",
	"123/12345678",
	"1234/1234567",
	"12345/123456",
	"123456/12345",
	"1234567/1234",
	"12345678/123",
	"123456789/12",
	"1234567890/1",
	"01234567890/",
	"123456789012",
};

/* Full horizontal reduction of an 8-wide 32-bit integer lanes vector */
static inline
uint32_t _mm256_reduce_epi32(__m256i datum)
{
	__m128i p1 = _mm_hadd_epi32(
		_mm256_extracti128_si256(datum, 0),
		_mm256_extracti128_si256(datum, 1));
	// TODO check if going through an intermediate _mm_hadd_pi32
	// is worth it
	return	_mm_extract_epi32(p1, 0) + _mm_extract_epi32(p1, 1) +
		_mm_extract_epi32(p1, 2) + _mm_extract_epi32(p1, 3);
}

/* Find the first 32-bit lane that is not masked by the given mask */
static inline
uint32_t first_unmasked_epi32(__m256i mask)
{
	return _tzcnt_u32( ~_mm256_movemask_epi8(mask) )  >> 2;
}

/* Find the first 32-bit lane that is masked by the given mask */
static inline
uint32_t first_masked_epi32(__m256i mask)
{
	return _tzcnt_u32( _mm256_movemask_epi8(mask) )  >> 2;
}


/* Parse a chunk of up to 8 consecutive ASCII decimal digits, returning
 * the corresponding numerical value, and setting `delta` to the number of
 * digits parsed
 */
uint32_t simd_parse_int_chunk(const char *from, uint32_t *parsed)
{
	/* Lower bound for 'is this a digit' */
	const __m256i lessthan0 = _mm256_set1_epi32('0' - 1);
	/* Upper bound for 'is this a digit' */
	const __m256i morethan9 = _mm256_set1_epi32('9' + 1);
	/* Turn ASCII digit into its value */
	const __m256i sub0 = _mm256_set1_epi32(-0x30);
	/* Lane index for permutation (base-1 because we will subtract this
	 * from the lane index of the first non-digit
	 */
	const __m256i laneidx = _mm256_set_epi32(8, 7, 6, 5, 4, 3, 2, 1);
	/* Scale multiplier for digit to its positiona value */
	const __m256i lanemul = _mm256_set_epi32(10000000, 1000000, 100000, 10000, 1000, 100, 10, 1);

	/* Load a fistful of digits */
	__m128i l128 = _mm_loadu_si128((const __m128i*)(from));
	__m256i l256 = _mm256_cvtepu8_epi32(l128);
	__m256i digits = _mm256_add_epi32(l256, sub0);

	/* Mask of the lanes that contain a valid digit (>= '0' && <= '9') */
	__m256i ge0 = _mm256_cmpgt_epi32(l256, lessthan0);
	__m256i le9 = _mm256_cmpgt_epi32(morethan9, l256);
	__m256i mask = _mm256_and_si256(ge0, le9);

	/* Index of the first invalid lane */
	const uint32_t parsable = first_unmasked_epi32(mask);

	/* Compute the permutation index for the scaling multipliers.
	 * This will put negative numbers in the lanes > first_invalid_lane */
	__m256i permutation = _mm256_sub_epi32(_mm256_set1_epi32(parsable), laneidx);;

	/* Selection of the _first_ lanes with valid digits, by masking against the
	 * lanes with non-negative value in `permutation` */
	__m256i valid = _mm256_andnot_si256(
		_mm256_cmpgt_epi32(_mm256_setzero_si256(), permutation),
		digits);

#if DEBUG_SIMD
	printf("%#8x %#8x %#8x %#8x %#8x %#8x %#8x %#8x ", EXPLODE_SIMD(valid));
	printf("%8d ", first_masked_epi32(mask));
	printf("%8d\n", parsable);
#endif
	/* Place the multipliers in the correct place */
	__m256i mulseq = _mm256_permutevar8x32_epi32(lanemul, permutation);
	/* Multiply */
	__m256i scaled = _mm256_mullo_epi32(valid, mulseq);
	/* Add */
	uint32_t sum = _mm256_reduce_epi32(scaled);

#if DEBUG_SIMD
	printf("%8d %8d %8d %8d %8d %8d %8d %8d ", EXPLODE_SIMD(scaled));
	printf("%8d\n", sum);
#endif

	*parsed = parsable;
	return sum;
}

static const uint32_t simd_parsed_scale[] = {
	1, 10, 100, 1000, 10000, 100000,
	1000000, 10000000, 100000000,
};

#define SIMD_DIGIT_LOOP(val, c, from) \
	char c = *from; \
	while (c >= '0' && c <= '9') { \
		uint32_t parsed = 0; \
		uint32_t chunk = simd_parse_int_chunk(from, &parsed); \
		val = val*simd_parsed_scale[parsed] + chunk; \
		from += parsed; \
		c = *from; \
	}

#define SIMD_PARSE(bits) \
static inline \
int##bits##_t simd_parse_int##bits(const char *from, char **end) \
{ \
	int##bits##_t val = 0; \
	int##bits##_t sign = 1; \
\
	CHECK_SIGN(sign, from) \
\
	SIMD_DIGIT_LOOP(val, c, from) \
\
	if (end) *end = (char*)from; \
	return sign*val; \
} \
static inline \
uint##bits##_t simd_parse_uint##bits(const char *from, char **end) \
{ \
	uint##bits##_t val = 0; \
\
	SIMD_DIGIT_LOOP(val, c, from) \
\
	if (end) *end = (char*)from; \
	return val; \
}

SIMD_PARSE(32)
SIMD_PARSE(64)

int main() {
	const size_t nbuffers = sizeof(buffer)/sizeof(*buffer);
	for (size_t b = 0; b < nbuffers; ++b) {
		printf("%s\n", buffer[b]);

		const char *from = NULL;
		char *end = (char*)(buffer[b]);

		from = end;
		uint64_t ret = simd_parse_uint64(from, &end);

		printf("Parsed: %" PRIu64 "\n", ret);
		printf("Rest: %s\n\n", end);

	}

}
