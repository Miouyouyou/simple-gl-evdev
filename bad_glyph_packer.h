#ifndef MYY_BAD_GLYPH_PACKER_H
#define MYY_BAD_GLYPH_PACKER_H 1

#include <stdint.h>

typedef uint32_t codepoint;

struct added_characters {
	uint32_t amount;
	codepoint list[];
};

struct character_infos {
	uint16_t striked_pt_size, pixels_per_line;
	uint16_t width, height;
	uint16_t bearing_x, bearing_y;
	uint16_t advance_x, advance_y;
};

struct index_found {
	uint32_t found;
	uint32_t i;
};

struct index_found myy_bgp_find_char_index
(struct added_characters const * restrict const added_chars,
 codepoint const checked_code);

unsigned int myy_bgp_add_char
(struct added_characters * restrict const chars,
 struct character_infos  * restrict const char_positions,
 codepoint const code_to_add,
 struct character_infos const pos);

unsigned int myy_bgp_new_char
(struct added_characters * restrict const chars,
 struct character_infos  * restrict const char_positions,
 codepoint const code_to_add,
 struct character_infos const pos);

void myy_bgp_print_char_infos
(struct character_infos const * restrict const char_data,
 codepoint const utf32_codepoint);

void myy_bgp_print_char_data
(struct added_characters const * restrict const added_chars,
 struct character_infos  const * restrict const char_positions,
 unsigned int const index);

#endif /* MYY_BAD_GLYPH_PACKER_H */
