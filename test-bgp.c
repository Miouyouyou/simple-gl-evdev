#include <ft2build.h>
#include FT_FREETYPE_H

#include <assert.h>
#include <wchar.h>

#include <helpers/log.h>

#include "bad_glyph_packer.h"

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

void assert_same_infos
(struct character_infos const * restrict const expected_metrics,
 struct character_infos const * restrict const checked_metrics) {
	assert(
	  memcmp(
	    expected_metrics, checked_metrics,
	    sizeof(struct character_infos)
	  ) == 0
	);
}

void assert_found_at
(struct added_characters const * restrict const added_chars,
 codepoint const charcode, unsigned int const expected_index) {

	struct index_found search_result =
	  myy_bgp_find_char_index(added_chars, charcode);

	assert(search_result.found == 1);
	assert(search_result.i == expected_index);

}

unsigned int assert_found
(struct added_characters const * restrict const added_chars,
 codepoint const charcode) {

	struct index_found search_result =
	  myy_bgp_find_char_index(added_chars, charcode);

	assert(search_result.found == 1);
	return search_result.i;

}

void assert_not_found
(struct added_characters const * restrict const added_chars,
 codepoint const charcode) {

	struct index_found search_result =
	  myy_bgp_find_char_index(added_chars, charcode);

	assert(search_result.found == 0);

}

inline void copy_26_6_metrics_in
(struct character_infos * const char_data,
 FT_Glyph_Metrics const * const glyph_metrics) {
	char_data->width     = glyph_metrics->width >> 6;
	char_data->height    = glyph_metrics->height >> 6;
	char_data->bearing_x = glyph_metrics->horiBearingX >> 6;
	char_data->bearing_y = glyph_metrics->horiBearingY >> 6;
	char_data->advance_x = glyph_metrics->horiAdvance >> 6;
	char_data->advance_y = glyph_metrics->vertAdvance >> 6;
}

extern void copy_26_6_metrics_in
(struct character_infos * const char_data,
 FT_Glyph_Metrics const * const glyph_metrics);

void test_add_char(FT_Library font_library, FT_Face font_face) {

	wchar_t * text = L"08!ABCDEF :　真・完全勝利";
	unsigned int text_size = wcslen(text);

	struct added_characters_simple {
		uint32_t amount;
		codepoint list[15];
	} added_chars_simple = {
		.amount = 0,
		.list = {0}
	};
	struct character_infos characters_data[15] = {0};

	struct added_characters * added_chars =
	  (struct added_characters *) &added_chars_simple;

	assert(added_chars->amount == 0);
	assert(added_chars->list[0] == 0);

	FT_GlyphSlot glyph = font_face->glyph;
	FT_Glyph_Metrics *glyph_metrics = &(glyph->metrics);
	struct character_infos current_char_infos;

	LOG("Testing adding glyph informations\n"
	    "Since each glyph added is unique, adding them should generate\n"
	    "new entries in the codepoint list and in the characters\n"
	    "list\n");
	for (unsigned int i = 0; i < text_size; i++) {
		codepoint to_add = text[i];
		assert(FT_Load_Char( font_face, to_add, FT_LOAD_NO_AUTOHINT ) == 0);
		show_bitmap(&glyph->bitmap, glyph->metrics.horiBearingX >> 6,
		            glyph->advance.x >> 6);

		copy_26_6_metrics_in(&current_char_infos, glyph_metrics);

		myy_bgp_print_char_infos(&current_char_infos, to_add);
		assert_not_found(added_chars, to_add);
		unsigned int char_i =
		  myy_bgp_add_char(added_chars, characters_data, to_add,
		                   current_char_infos);
		assert(added_chars->amount == i+1);
		assert(added_chars->list[char_i] == to_add);
		assert_found_at(added_chars, to_add, char_i);

		assert_same_infos(&current_char_infos, characters_data+char_i);

		myy_bgp_print_char_data(added_chars, characters_data, char_i);
	}

	LOG("Now that each glyph has been added, adding them again will just\n"
	    "return their index\n");
	for (unsigned int i = 0; i < text_size; i++) {
		codepoint to_add = text[i];
		assert(FT_Load_Char( font_face, to_add, FT_LOAD_NO_AUTOHINT) == 0);
		show_bitmap(&glyph->bitmap, glyph->metrics.horiBearingX >> 6,
		            glyph->advance.x >> 6);

		copy_26_6_metrics_in(&current_char_infos, glyph_metrics);

		myy_bgp_print_char_infos(&current_char_infos, to_add);

		assert_found(added_chars, to_add);
		unsigned int char_i =
		  myy_bgp_add_char(added_chars, characters_data, to_add,
		                   current_char_infos);
		assert(added_chars->amount == text_size);
		assert(added_chars->list[char_i] == to_add);
		assert_found_at(added_chars, to_add, char_i);

		assert_same_infos(&current_char_infos, characters_data+char_i);

		myy_bgp_print_char_data(added_chars, characters_data, char_i);
	}
}

int main() {
	FT_Library   font_library;
	FT_Face      font_face;

	assert(FT_Init_FreeType( &font_library ) == 0);
	assert(FT_New_Face( font_library, "Font.otf", 0, &font_face) == 0);
	assert(FT_Set_Char_Size( font_face, 16 * 64, 0, 100, 0 ) == 0);

	test_add_char(font_library, font_face);

	FT_Done_Face    ( font_face );
	FT_Done_FreeType( font_library );
	return 0;
}
