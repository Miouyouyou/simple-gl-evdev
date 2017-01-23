#ifndef MYY_PACKED_FONTS_PARSER_H
#define MYY_PACKED_FONTS_PARSER_H 1

#include <stdint.h>

struct myy_packed_fonts_info_header {
	uint32_t n_stored_codepoints;
	uint32_t codepoints_start_offset;
	uint32_t glyphdata_start_offset;
	uint32_t unused[4];
	uint32_t font_filename_size;
	uint8_t font_filename[32];
};

struct myy_packed_fonts_codepoints {
	uint32_t codepoint;
};

struct myy_packed_fonts_glyphdata {
	uint16_t tex_left, tex_right, tex_bottom, tex_top;
	int16_t offset_x_px, offset_y_px;
	int16_t advance_x_px, advance_y_px;
	uint16_t width_px, height_px;
};

struct glyph_infos {
	uint32_t stored_codepoints;
	struct myy_packed_fonts_codepoints *codepoints_addr;
	struct myy_packed_fonts_glyphdata  *glyphdata_addr;
};

void myy_parse_packed_fonts
(struct glyph_infos * __restrict const glyph_infos,
 char const * __restrict const filename);

#endif
