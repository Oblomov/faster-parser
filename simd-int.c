#define DEBUG_SIMD 1
#include "simd-int.h"

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
