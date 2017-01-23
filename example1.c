/* example1.c                                                      */
/*                                                                 */
/* This small program shows how to print a rotated string with the */
/* FreeType 2 library.                                             */

#include <stdio.h>
#include <string.h>
#include <math.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <wchar.h>
#include <stdint.h>

#include <assert.h>

#include "bad_glyph_packer.h"

#define WIDTH  640
#define HEIGHT 480


struct image_size {
  uint16_t width, height;
};

/* origin is the upper left corner */
uint8_t image[695][50] = {0};

struct empty_space {
  uint16_t width, height;
  uint16_t x, y;
} best_space = {
  .width  = 0,
  .height = 0,
  .x = 0,
  .y = 0
};

struct empty_space const final_bitmap = { 50, 695 };

struct added_characters added_chars = {
	.amount = 0,
	.list = {0}
};

struct character_infos characters_data[1024] = {0};

inline unsigned int can_put_in_best_remaining_space
(unsigned const width, unsigned const height) {
	return
		best_space.width  > width &&
		best_space.height > height;
}

inline void reduce_best_remaining_space
(unsigned const width, unsigned const height) {
  best_space.width -= width;
  best_space.height -= height;
  printf("Best remaining space reduced to : %d, %d\n", 
         best_space.width, best_space.height);
}

inline void new_best_remaining_space
(uint16_t const width, uint16_t const height, 
 uint16_t const x, uint16_t const y) {
  best_space.width = width;
  best_space.height = height;
  best_space.x = x;
  best_space.y = y;
  printf("New best remaining space : %d, %d (at: %d, %d)\n",
         width, height, x, y);
}

extern unsigned int can_put_in_best_remaining_space
(unsigned const width, unsigned const height);

extern void reduce_best_remaining_space
(unsigned const width, unsigned const height);

extern inline void new_best_remaining_space
(uint16_t const width, uint16_t const height,
 uint16_t const x, uint16_t const y);

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

void show_bitmap
( FT_Bitmap const * restrict const bitmap,
  unsigned int x_bearing,
  unsigned int padded_width ) {
	uint16_t b_width  = bitmap->width;
	uint16_t b_height = bitmap->rows;
	FT_Int  s, t, p;
	uint16_t r_margin = b_width + x_bearing;

	char *design_characters[] = {
	  " ", "░", "▒", "▓", "█"
	};
	for ( t = 0; t < b_height; t++ ) {
		int col_offset = t * b_width;

		for ( p = 0; p < x_bearing; p++ ) printf("X");
		for ( s = 0; p < r_margin; p++, s++ ) {
			unsigned int pixel = bitmap->buffer[col_offset + s];
			unsigned int c =
			  (pixel > 0x40) + (pixel > 0x80) + (pixel > 0xb0) +
			  (pixel == 0xff);
			printf("%s", design_characters[c]);

		}
		for (; p < padded_width; p++ ) printf("x");

		printf("|\n");
	}
	for (p = 0; p < padded_width; p++) printf("⁻");
	printf("|\n");
	fflush(stdout);
}



int main() {

	FT_Library   font_library;
	FT_Face      font_face;

	assert(FT_Init_FreeType( &font_library ) == 0);
	assert(FT_New_Face( font_library, "Font.otf", 0, &font_face) == 0);
	assert(FT_Set_Char_Size( font_face, 36 * 64, 0, 100, 0 ) == 0);

	assert(added_chars.amount == 0);
	assert(added_chars.list[0] == 0);

	wchar_t * text = L"Perfect Victory : 完全勝利";
	FT_GlyphSlot glyph = font_face->glyph;

	codepoint shown_char = L'👻';
	assert(FT_Load_Char( font_face, shown_char, FT_LOAD_RENDER ) == 0);


	printf("bitmap width : %d, height : %d\n",
	        glyph->bitmap.width, glyph->bitmap.rows );
	show_bitmap(&glyph->bitmap, glyph->metrics.horiBearingX >> 6,
	            glyph->advance.x >> 6);

  FT_Glyph_Metrics *glyph_metrics = &(glyph->metrics);
	struct character_infos Shin = {
		.width  = glyph_metrics->width >> 6,
		.height = glyph_metrics->height >> 6,
		.bearing_x = glyph_metrics->horiBearingX >> 6,
		.bearing_y = glyph_metrics->horiBearingY >> 6,
		.advance_x = glyph_metrics->horiAdvance >> 6,
		.advance_y = glyph_metrics->vertAdvance >> 6,
	};

	print_char_infos(&Shin, shown_char);

	struct index_found found_char =
	  myy_bgp_find_char_index(&added_chars, shown_char);
	assert(found_char.found == 0);

	myy_bgp_add_char(&added_chars, characters_data, shown_char, Shin);

	assert(added_chars.amount == 1);
	assert(added_chars.list[0] == shown_char);
	found_char = myy_bgp_find_char_index(&added_chars, shown_char);

	assert(found_char.found == 1);
	assert(found_char.i     == 0);

	unsigned int char_i = found_char.i;
	assert(Shin.width     == characters_data[char_i].width);
	assert(Shin.height    == characters_data[char_i].height);
	assert(Shin.bearing_x == characters_data[char_i].bearing_x);
	assert(Shin.bearing_y == characters_data[char_i].bearing_y);
	assert(Shin.advance_x == characters_data[char_i].advance_x);
	assert(Shin.advance_y == characters_data[char_i].advance_y);
	FT_Done_Face    ( font_face );
	FT_Done_FreeType( font_library );

}

void
show_image( void ) {
	int f_width  = final_bitmap.width,
	    f_height = final_bitmap.height;

	for (unsigned int t = 0; t < f_height; t++) {
		for (unsigned int s = 0; s < f_width; s++)
			printf("%02x,", image[t][s]);

		printf("\n");
	}
}

/* EOF */
