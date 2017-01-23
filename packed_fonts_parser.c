#include <packed_fonts_parser.h>

#include <helpers/file.h>
#include <helpers/log.h>
#include <helpers/string.h>

#include <string.h>

static void print_stored_codepoints_infos
(struct glyph_infos const * __restrict const glyph_infos) {
	unsigned int n_codepoints = glyph_infos->stored_codepoints;
	struct myy_packed_fonts_glyphdata const * __restrict const glyphs =
	  glyph_infos->glyphdata_addr;
	struct myy_packed_fonts_codepoints const * __restrict const codepoints =
	  glyph_infos->codepoints_addr;

	struct myy_packed_fonts_glyphdata const * __restrict current_glyph;
	uint32_t current_codepoint;
	char converted_codepoint[8] = {0};
	LOG("[print_stored_codepoints_infos]\n");
	for (unsigned int i = 0; i < n_codepoints; i++) {
		memset(converted_codepoint, 0, 8);
		current_codepoint = codepoints[i].codepoint;
		utf32_to_utf8_string(current_codepoint, converted_codepoint);
		current_glyph = glyphs+i;
		LOG("  %s (%d) ↓\n"
		    "  Tex: left: %d, right: %d, bottom: %d, top: %d\n"
		    "  Off: x: %dpx, y: %dpx\n"
		    "  Siz: width: %d px, height: %d px\n",
		    converted_codepoint, current_codepoint,
		    current_glyph->tex_left,    current_glyph->tex_right,
		    current_glyph->tex_bottom,  current_glyph->tex_top,
		    current_glyph->offset_x_px, current_glyph->offset_y_px,
		    current_glyph->width_px,    current_glyph->height_px
		);
	}
}

void myy_parse_packed_fonts
(struct glyph_infos * __restrict const glyph_infos,
 char const * __restrict const filename) {

	struct myy_packed_fonts_info_header header;
	fh_ReadBytesFromFile(
	  filename, (uint8_t *) &header, sizeof(header), 0
	);

	LOG("[myy_parse_packed_fonts]\n"
	    "  filename                   : %s\n"
	    "  Codepoints stored          : %d\n"
	    "  Codepoints starting offset : %d\n"
	    "  Glyph data start offset    : %d\n"
	    "  Linked Font filename size  : %d\n"
	    "  Linked Font filename       : %s\n",
	    filename,
	    header.n_stored_codepoints,
	    header.codepoints_start_offset,
	    header.glyphdata_start_offset,
	    header.font_filename_size,
	    header.font_filename
	);

	fh_ReadBytesFromFile(
	  filename, (uint8_t *) glyph_infos->codepoints_addr,
	  header.n_stored_codepoints *
	  sizeof(struct myy_packed_fonts_codepoints),
	  header.codepoints_start_offset
	);

	fh_ReadBytesFromFile(
	  filename, (uint8_t *) glyph_infos->glyphdata_addr,
	  header.n_stored_codepoints *
	  sizeof(struct myy_packed_fonts_glyphdata),
	  header.glyphdata_start_offset
	);

	glyph_infos->stored_codepoints = header.n_stored_codepoints;

	print_stored_codepoints_infos(glyph_infos);
}

