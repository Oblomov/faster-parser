#include <immintrin.h>
#include <inttypes.h>
#include <stdio.h>

#define DEBUG_SIMD 1
#define LANE8(v, lane) _mm256_extract_epi32(v, lane)
#define EXPLODE_SIMD(v) \
	LANE8(v, 0), LANE8(v, 1), LANE8(v, 2), LANE8(v, 3), \
	LANE8(v, 4), LANE8(v, 5), LANE8(v, 6), LANE8(v, 7)

const char *buffer[] = {
	"/01234567890",
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
};

/* Full horizontal reduction of an 8-wide 32-bit integer lanes vector */
int32_t _mm256_reduce_epi32(__m256i datum)
{
	__m128i p1 = _mm_hadd_epi32(
		_mm256_extracti128_si256(datum, 0),
		_mm256_extracti128_si256(datum, 1));
	// TODO check if going through an intermediate _mm_hadd_pi32
	// is worth it
	return	_mm_extract_epi32(p1, 0) + _mm_extract_epi32(p1, 1) +
		_mm_extract_epi32(p1, 2) + _mm_extract_epi32(p1, 3);
}

int32_t simd_parse_int(const char *from, char **end)
{
	/* Lower bound for 'is this a digit' */
	const __m256i lessthan0 = _mm256_set1_epi32('0' - 1);
	/* Upper bound for 'is this a digit' */
	const __m256i morethan9 = _mm256_set1_epi32('9' + 1);
	/* Turn ASCII digit into its value */
	const __m256i sub0 = _mm256_set1_epi32(-0x30);
	/* Lane index for permutation */
	const __m256i laneidx = _mm256_set_epi32(7, 6, 5, 4, 3, 2, 1, 0);
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
	const uint32_t invalid_bitmask = ~_mm256_movemask_epi8(mask);
	const uint32_t first_invalid_lane = _tzcnt_u32(invalid_bitmask) >> 2;

	/* Compute the permutation index for the scaling multipliers.
	 * This will put negative numbers in the lanes > first_invalid_lane */
	__m256i permutation = _mm256_sub_epi32(_mm256_set1_epi32(first_invalid_lane-1), laneidx);;

	/* Selection of the _first_ lanes with valid digits */
	__m256i valid = _mm256_andnot_si256(_mm256_cmpgt_epi32(_mm256_setzero_si256(), permutation), digits);

#if DEBUG_SIMD
	printf("%#8x\t%#8x\t%#8x\t%#8x\t%#8x\t%#8x\t%#8x\t%#8x\t", EXPLODE_SIMD(valid));
	printf("%#8x\t", invalid_bitmask);
	printf("%8d\n", first_invalid_lane);
#endif
	/* Place the multipliers in the correct place */
	__m256i mulseq = _mm256_permutevar8x32_epi32(lanemul, permutation);
	/* Multiply */
	__m256i scaled = _mm256_mullo_epi32(valid, mulseq);
	/* Add */
	int32_t sum = _mm256_reduce_epi32(scaled);

#if DEBUG_SIMD
	printf("%8d\t%8d\t%8d\t%8d\t%8d\t%8d\t%8d\t%8d\t", EXPLODE_SIMD(scaled));
	printf("%8d\n", sum);
#endif

	if (end) *end = (char*)from + first_invalid_lane;
	return sum;
}

int main() {

	const size_t nbuffers = sizeof(buffer)/sizeof(*buffer);
	for (size_t b = 0; b < nbuffers; ++b) {
		printf("%s\n", buffer[b]);

		const char *from = NULL;
		char *end = (char*)(buffer[b]);

		from = end;
		int32_t ret = simd_parse_int(from, &end);

		printf("%s", end);
		printf("%" PRId32 "\n", ret);

	}

}
