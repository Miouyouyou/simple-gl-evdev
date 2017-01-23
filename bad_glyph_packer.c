#include "bad_glyph_packer.h"
#include <helpers/log.h>

uint32_t utf32_to_utf8(uint32_t code) { // 00000000 0xxxxxxx
	uint32_t utf8_codepoint;
	if (code < 0x80) utf8_codepoint = code;
	else if (code < 0x800) {   // 00000yyy yyxxxxxx
		utf8_codepoint =
		  (0b11000000 | (code >> 6)   ) << 8 |
		  (0b10000000 | (code & 0x3f) );
	}
	else if (code < 0x10000) {  // zzzzyyyy yyxxxxxx
		utf8_codepoint =
		  (0b11100000 | (code >> 12)         ) << 16 |  // 1110zzz
		  (0b10000000 | ((code >> 6) & 0x3f) ) << 8  |  // 10yyyyy
		  (0b10000000 | (code & 0x3f)        );         // 10xxxxx
	}
	else if (code < 0x200000) { // 000uuuuu zzzzyyyy yyxxxxxx
		utf8_codepoint =
		  (0b11110000 | (code >> 18)          ) << 24 | // 11110uuu
		  (0b10000000 | ((code >> 12) & 0x3f) ) << 16 | // 10uuzzzz
		  (0b10000000 | ((code >> 6)  & 0x3f) ) << 8  | // 10yyyyyy
		  (0b10000000 | (code & 0x3f)         );        // 10xxxxxx
	}
}

void utf32_to_utf8_string(uint32_t code, char * string) {
	if (code < 0x80) string[0] = code;
	else if (code < 0x800) {   // 00000yyy yyxxxxxx
		string[0] = (0b11000000 | (code >> 6));
		string[1] = (0b10000000 | (code & 0x3f));
	}
	else if (code < 0x10000) {  // zzzzyyyy yyxxxxxx
		string[0] = (0b11100000 | (code >> 12));         // 1110zzz
		string[1] = (0b10000000 | ((code >> 6) & 0x3f)); // 10yyyyy
		string[2] = (0b10000000 | (code & 0x3f));        // 10xxxxx
	}
	else if (code < 0x200000) { // 000uuuuu zzzzyyyy yyxxxxxx
		string[0] = (0b11110000 | (code >> 18));          // 11110uuu
		string[1] = (0b10000000 | ((code >> 12) & 0x3f)); // 10uuzzzz
		string[2] = (0b10000000 | ((code >> 6)  & 0x3f)); // 10yyyyyy
		string[3] = (0b10000000 | (code & 0x3f));         // 10xxxxxx
	}
}

struct index_found myy_bgp_find_char_index
(struct added_characters const * restrict const added_chars,
 codepoint const checked_code) {

	unsigned int i = 0;
	codepoint const * restrict const char_list = added_chars->list;

	while( char_list[i] && char_list[i] != checked_code ) i++;

	struct index_found found = {
		.found = char_list[i] == checked_code,
		.i = i
	};
	return found;

}

unsigned int myy_bgp_new_char
(struct added_characters * restrict const chars,
 struct character_infos  * restrict const char_positions,
 codepoint const code_to_add, struct character_infos const pos) {

	unsigned int last_index = chars->amount++;
	chars->list[last_index] = code_to_add;
	char_positions[last_index] = pos;
	return last_index;

}

unsigned int myy_bgp_add_char
(struct added_characters * restrict const chars,
 struct character_infos  * restrict const char_positions,
  codepoint const code_to_add, struct character_infos const pos) {

	struct index_found search_result =
	  myy_bgp_find_char_index(chars, code_to_add);

	unsigned int char_i;
	if (!search_result.found)
		char_i = myy_bgp_new_char(chars, char_positions, code_to_add, pos);
	else char_i = search_result.i;

	return char_i;
}


void myy_bgp_print_char_infos
(struct character_infos const * const char_data,
 codepoint const utf32_codepoint) {
	char utf8_string[5] = {0};
	utf32_to_utf8_string(utf32_codepoint, utf8_string);

	LOG("Saved metrics for %s : \n"
	    "  width  : %d\n"
	    "  height : %d\n"
	    "  horizontal bearing x : %d\n"
	    "  horizontal bearing y : %d\n"
	    "  advance.x : %d\n"
	    "  advance.y : %d\n",
	    utf8_string,
	    char_data->width, char_data->height,
	    char_data->bearing_x, char_data->bearing_y,
	    char_data->advance_x, char_data->advance_y);
}

void myy_bgp_print_char_data
(struct added_characters const * restrict const added_chars,
 struct character_infos  const * restrict const char_positions,
 unsigned int const index) {
	if (index < added_chars->amount)
		myy_bgp_print_char_infos(
		  char_positions+index,
		  added_chars->list[index]
		);
}

