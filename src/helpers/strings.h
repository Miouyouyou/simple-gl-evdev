#ifndef MYY_HELPERS_STRINGS_H
#define MYY_HELPERS_STRINGS_H 1

#include <helpers/log.h>

#include <stdint.h>

struct utf8_codepoint {
	uint32_t value;
	uint32_t size;
};

__attribute__((unused))
static struct utf8_codepoint utf8_codepoint_and_size(
	uint8_t const * __restrict const string)
{
	uint8_t current_char = string[0];
	unsigned int codepoint = 0;
	unsigned int size = 1;

	unsigned int const multibyte_mask = 0b00111111;

	if (current_char < 0x80)
		codepoint = current_char;
	else if (current_char >> 5 == 0b110) {
		codepoint = (current_char & 0b00011111) << 6;
		codepoint |= (string[1] & multibyte_mask);
		size = 2;
	}
	else if (current_char >> 4 == 0b1110) {
		codepoint = (current_char & 0b00001111) << 12;
		codepoint |= (string[1] & multibyte_mask) << 6;
		codepoint |= (string[2] & multibyte_mask);
		size = 3;
	}
	else if (current_char >> 3 == 0b11110) {
		codepoint = (current_char & 0b00001111) << 18;
		codepoint |= (string[1] & multibyte_mask) << 12;
		codepoint |= (string[2] & multibyte_mask) << 6;
		codepoint |= (string[3] & multibyte_mask);
		size = 4;
	}
	else LOG("Malformed char : %d\n", current_char);

	struct utf8_codepoint converted_codepoint = {
		.value = codepoint,
		.size = size
	};
	
	return converted_codepoint;
}

#endif