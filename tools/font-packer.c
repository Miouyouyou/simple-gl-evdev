#include <freetype2/ft2build.h>
#include FT_FREETYPE_H

#include <stdint.h> // (u)int*_t

#include <sys/types.h> // open
#include <sys/stat.h>  // open
#include <fcntl.h>     // open

#include <unistd.h>    // read, close
#include <stdbool.h>
#undef NDEBUG

#include <helpers/strings.h>
#include <helpers/file.h>
#include <helpers/fonts/packed_fonts_parser.h>
#include <helpers/opengl/loaders.h>

#include <helpers/log.h>
#include <helpers/myy_vector/vector.h>

#define max(a,b) ((a) > (b) ? (a) : (b))
#define min(a,b) ((a) < (b) ? (a) : (b))

#define FONT_SIZE (20)



static inline uint16_t normalized_u16(
	uint_fast32_t size, uint_fast32_t total_size, double offset)
{
	double offseted_size = offset + size;
	return (uint16_t) (offseted_size * 65535.0 / total_size);;
}

static inline uint16_t renormalize_u16(
	uint16_t * __restrict const value,
	uint_fast32_t const total_size,
	double offset)
{
	uint_fast32_t current_value = *value;
	*value = normalized_u16(current_value, total_size, offset);
}

static void normalize_tex_coordinates(
	struct myy_packed_fonts_glyphdata * __restrict const glyphdata,
	uint_fast32_t total_width,
	uint_fast32_t total_height)
{
	renormalize_u16(&glyphdata->tex_left,   total_width,  0);
	renormalize_u16(&glyphdata->tex_right,  total_width,  0);
	renormalize_u16(&glyphdata->tex_bottom, total_height, 0);
 	renormalize_u16(&glyphdata->tex_top,    total_height, 0);
}

enum program_status {
	status_ok,
	status_not_enough_arguments,
	status_cannot_open_font,
	status_could_not_initialize_fonts,
	status_not_enough_initial_memory_for_faces,
	status_not_enough_initial_memory_for_bitmaps_metadata,
	status_not_enough_initial_memory_for_bitmaps,
	status_not_enough_initial_memory_for_codepoints,
	status_font_filepath_does_not_exist,
	status_chars_filepath_does_not_exist,
	status_could_not_load_codepoints_from_file,
	status_could_not_open_bitmap_output_file,
	status_could_not_open_metadata_output_file,
	status_could_not_write_bitmap_file,
	status_could_not_write_metadata_file,
	status_could_not_get_a_bitmap_for_each_character,
	n_status
};

char const * __restrict const status_messages[n_status] = {
	[status_ok]                      = "",
	[status_not_enough_arguments]    =
		"Not enough arguments\n",
	[status_cannot_open_font]        =
		"The font could not be opened %m\n",
	[status_could_not_initialize_fonts] =
		"Failed to initialize the fonts with FreeType\n",
	[status_not_enough_initial_memory_for_faces] =
		"Not enough initial memory for Freetype Font Faces\n",
	[status_not_enough_initial_memory_for_bitmaps_metadata] =
		"Not enough initial memory for bitmaps metadata\n",
	[status_not_enough_initial_memory_for_bitmaps] =
		"Not enough initial memory for bitmaps\n",
	[status_not_enough_initial_memory_for_codepoints] =
		"Not enough initial memory for codepoints\n",
	[status_font_filepath_does_not_exist] =
		"The font filepath does not exist\n",
	[status_chars_filepath_does_not_exist] =
		"The chars filepath does not exist\n",
	[status_could_not_load_codepoints_from_file] =
		"Could not load the codepoints for the chars file\n",
	[status_could_not_open_bitmap_output_file] =
		"Could not create or write into the bitmap output file\n",
	[status_could_not_open_metadata_output_file] =
		"Could not create or write into the metadata output file\n",
	[status_could_not_write_bitmap_file] =
		"Failed to write the whole bitmap file\n",
	[status_could_not_write_metadata_file] =
		"Failed to write the whole metadata file\n",
	[status_could_not_get_a_bitmap_for_each_character] =
		"Failed to load a bitmap for each character.\n"
		"Try to provide more fonts\n"
		"or at least one font with a default (0) character.\n"
};

struct point_i16 {
	int16_t x, y;
};

struct bitmap_metadata {
	uint16_t width;
	uint16_t height;
	uint16_t stride;
	uint16_t size;
	uint8_t * data_address;
	uint32_t index;
};
myy_vector_template(bitmap_metadata, struct bitmap_metadata)
myy_vector_template(glyph_metadata, struct myy_packed_fonts_glyphdata)
myy_vector_template(u8, uint8_t)
myy_vector_template(u32, uint32_t)
myy_vector_template(ft_face, FT_Face)

#if !defined(NDEBUG)
static inline void print_pixel(uint_fast8_t pixel_value)
{
	struct {
		uint8_t size;
		uint8_t codepoints[4];
	} states[5] = {
		[0] = {1,{' '}},
		[1] = {3,{226, 150, 145}}, // ░
		[2] = {3,{226, 150, 146}}, // ▒
		[3] = {3,{226, 150, 147}}, // ▓
		[4] = {3,{226, 150, 136}}  // █
	};
	uint_fast8_t index = (pixel_value + 1) / 64;
	write(fileno(stdout), states[index].codepoints, states[index].size);
}
#else
#define print_pixel(pixel_value) 0
#endif

static bool load_char(FT_Face face, uint32_t utf32_code)
{
	if (FT_Get_Char_Index(face, utf32_code) == 0)
		LOG("%u not found\n", utf32_code);

	int error = FT_Load_Char(face, utf32_code, FT_LOAD_DEFAULT);
	if (error != 0) {
		LOG("Could not get character %c : 0x%02x\n",
			(uint8_t) utf32_code, error);
		goto could_not_load_character;
	}

	FT_GlyphSlot const glyph_slot = face->glyph;

	error = FT_Render_Glyph(glyph_slot, FT_RENDER_MODE_NORMAL);

	if (error != 0) {
		LOG("Could not render character %c : 0x%02x\n",
			(uint8_t) utf32_code, error);
		goto could_not_render_glyph;
	}

could_not_render_glyph:
could_not_load_character:
	return error == 0;

}

bool copy_char(
	FT_Face const face,
	int32_t max_height,
	myy_vector_u8 * __restrict const bitmaps,
	myy_vector_bitmap_metadata * __restrict const bitmaps_metadata,
	myy_vector_glyph_metadata * __restrict const glyphs_metadata,
	int16_t * __restrict const min_bearing_y)
{
	bool ret = false;
	FT_GlyphSlot const glyph   = face->glyph;
	FT_Bitmap const bitmap     = glyph->bitmap;

	*min_bearing_y = max(*min_bearing_y, (glyph->metrics.horiBearingY >> 6));
	LOG("Min_bearing_y : %d\n", (glyph->metrics.horiBearingY >> 6));

	struct myy_packed_fonts_glyphdata glyph_metadata = {
		.offset_x_px =
			glyph->metrics.horiBearingX >> 6,
		.offset_y_px = 
			(glyph->metrics.horiBearingY - glyph->metrics.height) >> 6,
		.advance_x_px = glyph->advance.x >> 6,
		/* Why store this everytime instead of storing it one time...
		 * Hmm...
		 * The questions would be :
		 * - Where to store it then ?
		 * - Is there real benefits to storing it once ?
		 * The data will all be loaded AND used at once
		 * during display.
		 */
		.advance_y_px = max_height,
		.width_px = bitmap.width,
		.height_px = bitmap.rows
	};

	struct bitmap_metadata metadata = {
		.width  = bitmap.width,
		.height = bitmap.rows,
		.stride = bitmap.pitch,
		.size   = bitmap.rows * bitmap.pitch,
		.index  =
			myy_vector_bitmap_metadata_length(bitmaps_metadata)
	};

	ret = 
		myy_vector_bitmap_metadata_add(bitmaps_metadata, 1, &metadata);

	if (!ret) {
		LOG("Could not add metadata\n");
		goto could_not_add_the_bitmap_metadata;
	}

	ret = myy_vector_glyph_metadata_add(glyphs_metadata,
		1, &glyph_metadata);
	if (!ret) {
		LOG("Could not add the glyph metadata\n");
		goto could_not_add_the_glyph_metadata;
	}

	ret = myy_vector_u8_add(bitmaps, metadata.size,
		bitmap.buffer);
	if (!ret) {
		LOG("Could not generate the bitmap\n");
		goto could_not_add_the_bitmap;
	}

	return true;

could_not_add_the_bitmap:
	myy_vector_glyph_metadata_forget_last(glyphs_metadata, 1);
could_not_add_the_glyph_metadata:
	myy_vector_bitmap_metadata_forget_last(bitmaps_metadata, 1);
could_not_add_the_bitmap_metadata:
	return ret;
}

static FT_Face first_face_that_can_display(
	myy_vector_ft_face const * __restrict const faces,
	uint32_t codepoint)
{
	myy_vector_for_each(faces, FT_Face, face, {
		LOG("Searching %u in %s... ", codepoint, face->family_name);
		if (FT_Get_Char_Index(face, codepoint)) {
			LOG("OK !\n");
			return face;
		}
		LOG("Nope.\n");
	});
	LOG("No provided font can display : %u\n",
		codepoint);
	return NULL;
}

bool copy_default_char(
	myy_vector_ft_face const * __restrict const faces,
	int32_t max_height,
	myy_vector_u8 * __restrict const bitmaps,
	myy_vector_bitmap_metadata * __restrict const bitmaps_metadata,
	myy_vector_glyph_metadata * __restrict const glyphs_metadata,
	int16_t * __restrict min_bearing_y)
{
	bool ret = false;
	/* 0 is the default character on some font.
	 * However, not every font has such character
	 */
	FT_Face face = first_face_that_can_display(faces, 0);
	if (face != NULL && load_char(face, 0)) {
		ret = copy_char(
			face,
			max_height,
			bitmaps, bitmaps_metadata,
			glyphs_metadata,
			min_bearing_y);
	}
	return ret;
}

static void add_addresses_of_each_bitmap(
	myy_vector_bitmap_metadata const * __restrict const bitmaps_metadata,
	myy_vector_u8 * __restrict const bitmaps)
{
	uint8_t * __restrict cursor =
		myy_vector_u8_data(bitmaps);

	int i = 0;
	myy_vector_for_each_ptr(
		struct bitmap_metadata, metadata, in, bitmaps_metadata,
		{
			metadata->data_address = cursor;
			cursor += metadata->size;
			i++;
		}
	);
	LOG("%d bitmaps\n", i);
}

uint32_t max_height_of_all_faces(
	myy_vector_ft_face const * __restrict const faces)
{
	uint32_t max_height = 0;
	myy_vector_for_each(faces, FT_Face, face, {
		max_height = 
			max(max_height, (face->size->metrics.height >> 6));
	});
	return max_height;
}

bool compute_each_bitmap_individually(
	myy_vector_ft_face const * __restrict const faces,
	myy_vector_u32 const * __restrict const codepoints,
	myy_vector_u8 * __restrict const bitmaps,
	myy_vector_bitmap_metadata * __restrict const bitmaps_metadata,
	myy_vector_glyph_metadata * __restrict const glyph_metadata,
	int16_t * __restrict const min_bearing_y)
{
	uint32_t const max_height =
		max_height_of_all_faces(faces);
	int32_t const advance_y = -max_height;
	bool ret = false;
	myy_vector_for_each(codepoints, uint32_t, codepoint, {
		FT_Face face =
			first_face_that_can_display(faces, codepoint);
		if (face != NULL && load_char(face, codepoint)) {
			ret = copy_char(
				face,
				advance_y,
				bitmaps, bitmaps_metadata,
				glyph_metadata,
				min_bearing_y);
			if (!ret) {
				printf(
					"  Something went wrong when copying glyph for codepoint %u\n"
					"using the font %s\n",
					codepoint,
					face->family_name != NULL ? face->family_name : "<No name>");
				goto could_not_copy_character;
			}
		}
		else {
			printf(
				"Trying to load a default character for codepoint %u\n",
				codepoint);
			ret = copy_default_char(
				faces,
				advance_y,
				bitmaps, bitmaps_metadata,
				glyph_metadata,
				min_bearing_y);
			if (!ret) {
				printf(
					"  Could not load a default character for codepoint %u\n",
					codepoint);
				goto could_not_load_default_character;
			}
		}
	});
	/* Since the bitmaps have flexible sizes and the
	 * vector used to store them can expand when adding
	 * data to it, and change its base address when
	 * expanding, we compute the addresses of each bitmap
	 * at the end.
	 * Every metadata index is relative to the bitmap index.
	 * Then the procedure is roughly like this :
	 * Set the cursor to the packed bitmaps buffer start
	 * for each metadata :
	 *   Copy metadata.size octets from the bitmaps buffer.
	 *   Advance the cursor by metadata.size octets
	 */
	add_addresses_of_each_bitmap(bitmaps_metadata, bitmaps);

	return ret;
could_not_copy_character:
could_not_load_default_character:
	return ret;
}

/* TODO
 * Integrate min_bearing_y to this.
 * Store these information in the headers.
 */
struct global_statistics {
	size_t total_height;
	size_t max_width;
};

struct global_statistics bitmaps_statistics(
	myy_vector_bitmap_metadata const * __restrict const bitmaps_metadata)
{
	struct global_statistics stats = {0, 0};

	myy_vector_for_each(
		bitmaps_metadata, struct bitmap_metadata, metadata,
		{
			/* Compute statistics */
			stats.total_height += metadata.height;
			stats.max_width    = max(stats.max_width, metadata.width);
		}
	);

	return stats;
}

void print_bitmaps(
	myy_vector_bitmap_metadata const * __restrict const bitmaps_metadata)
{

	size_t total_height = 0;
	size_t max_width    = 0;

	myy_vector_for_each(
		bitmaps_metadata, struct bitmap_metadata, metadata,
		{
			uint8_t const * __restrict pixels =
				metadata.data_address;

			/* Compute statistics */
			total_height += metadata.height;
			max_width    = max_width > metadata.width
				? max_width
				: metadata.width;

			/* Print the pixels */
			LOG("\n");
			for (uint16_t h = 0; h < metadata.height; h++)
			{
				for (uint16_t w = 0; w < metadata.width; w++)
				{
					print_pixel(pixels[w]);
				}
				LOG("\n");
				pixels += metadata.stride;
			}
			LOG("\n");
		}
	);

	LOG(
		"Total height : %lu\n"
		"Max width    : %lu\n",
		total_height, max_width);
}



uint_fast8_t fh_file_exist(char const * __restrict const filepath)
{
	int fd = open(filepath, O_RDONLY);
	int could_open_file = (fd >= 0);
	if (could_open_file)	close(fd);
	return could_open_file;
}

static void print_usage(char const * __restrict const program_name)
{
	fprintf(stderr, 
		"Usage : %s /path/to/font/file.ttf [/path/to/other/font.otf, ...] /path/to/chars.txt\n",
		program_name);
}

static bool chars_to_codepoints(
	myy_vector_u32 * __restrict const codepoints,
	uint8_t const * __restrict const utf8_chars,
	size_t const utf8_chars_size)
{
	uint8_t const * __restrict cursor = utf8_chars;
	uint8_t const * __restrict const end =
		cursor + utf8_chars_size;

	uint32_t i = 0;
	while(cursor < end) {
		struct utf8_codepoint codepoint =
			utf8_codepoint_and_size(cursor);

		bool added = myy_vector_u32_add(codepoints, 1, &codepoint.raw);

		if (!added) break;

		cursor += codepoint.size;
		i++;
	}

	return cursor >= end;
}

static int compare_codepoints(void const * pa, void const * pb)
{
	uint32_t const a = *((uint32_t const *) pa);
	uint32_t const b = *((uint32_t const *) pb);

	return a - b;
}

static void uniq(
	myy_vector_u32 * __restrict const codepoints_data)
{
	size_t vector_size = myy_vector_u32_length(codepoints_data);

	uint32_t * const sorted_codepoints =
		myy_vector_u32_data(codepoints_data);
	uint32_t const * const codepoints =
		myy_vector_u32_data(codepoints_data);

	uint32_t uniques = 1;

	for (uint32_t before = 0, i = 1; i < vector_size; before++, i++)
	{
		uint32_t codepoint = codepoints[i];
		if (codepoint != codepoints[before])
			sorted_codepoints[uniques++] = codepoint;
	}

	codepoints_data->tail =
		(uintptr_t) (sorted_codepoints + uniques);
}

static bool load_codepoints_from_chars_file(
	myy_vector_u32 * __restrict const codepoints,
	char const * __restrict const chars_filepath)
{
	bool ret = false;

	if (!fh_file_exist(chars_filepath))
		goto cannot_open_chars;

	struct myy_fh_map_handle mapping_result = 
		fh_MapFileToMemory(chars_filepath);

	if (!mapping_result.ok)
		goto cannot_map_file;

	ret = chars_to_codepoints(
		codepoints, mapping_result.address, mapping_result.length);

	fh_UnmapFileFromMemory(mapping_result);

	if (!ret)
		goto could_not_convert_all_codepoints;

	qsort(myy_vector_u32_data(codepoints),
		myy_vector_u32_length(codepoints),
		sizeof(uint32_t),
		compare_codepoints);

	uniq(codepoints);

	LOG("%lu\n", myy_vector_u32_length(codepoints));
could_not_convert_all_codepoints:
cannot_map_file:
cannot_open_chars:
	return ret;
}

static bool fonts_deinit(
	FT_Library * library,
	myy_vector_ft_face * __restrict const faces)
{
	myy_vector_for_each(faces, FT_Face, face_to_delete, {
		FT_Done_Face(face_to_delete);
	});
	FT_Done_FreeType(*library);
}

static bool fonts_init(
	FT_Library * library,
	char const * __restrict const * __restrict const fonts_filepath,
	uint32_t n_fonts,
	myy_vector_ft_face * __restrict const faces)
{

	if (FT_Init_FreeType( library ) != 0)
		goto ft_init_failed;

	for (uint32_t i = 0; i < n_fonts; i++) {
		FT_Face face;
		if (FT_New_Face( *library, fonts_filepath[i], 0, &face) != 0)
			goto could_init_all_fonts;

		/* Add the face one initialized, so that we can free it
		 * ASAP if we fail afterwards
		 */
		myy_vector_ft_face_add(faces, 1, &face);

		if (FT_Set_Char_Size(face, FONT_SIZE*64, 0, 96, 0) != 0)
			goto could_not_set_char_size_on_all_fonts;

	}

	return true;

could_not_set_char_size_on_all_fonts:
could_init_all_fonts:
	fonts_deinit(library, faces);
ft_init_failed:
	return false;
}

static int compare_bitmaps_by_width(void const * pa, void const * pb)
{
	struct bitmap_metadata * a = (struct bitmap_metadata *) pa;
	struct bitmap_metadata * b = (struct bitmap_metadata *) pb;

	return a->width - b->width;
}

static void sort_bitmaps_metadata_by_width(
	myy_vector_bitmap_metadata * __restrict const metadata)
{
	qsort(myy_vector_bitmap_metadata_data(metadata),
		myy_vector_bitmap_metadata_length(metadata),
		myy_vector_bitmap_metadata_type_size(),
		compare_bitmaps_by_width);
}



/* TODO : Padding is hard coded right now.
 * Provide the arguments for left and right padding.
 * Use them in the utility functions
 */
static inline uint8_t * add_padding_left(uint8_t * __restrict texture)
{
	*texture++ = 0;
	return texture;
}

/* TODO : Padding is hard coded right now.
 * Provide the arguments for left and right padding.
 * Use them in the utility functions
 */
static inline uint8_t * add_padding_right(uint8_t * __restrict texture)
{
	*texture++ = 0;
	return texture;
}

static inline uint8_t * add_lower_padding(
	uint8_t * __restrict texture,
	uint16_t texture_width)
{
	while(texture_width--)
		*texture++ = 0;

	return texture;
}

static inline uint8_t * add_upper_padding(
	uint8_t * __restrict texture,
	uint16_t texture_width)
{
	while(texture_width--)
		*texture++ = 0;

	return texture;
}

/* TODO : Padding is hard coded right now.
 * Provide the arguments for left and right padding.
 * Use them in the utility functions
 */
static inline void blit(
	uint8_t * __restrict dst,
	uint16_t const dst_width,
	struct bitmap_metadata bitmap_infos)
{
	uint8_t const * __restrict bitmap_cursor =
		(uint8_t const * __restrict)
		bitmap_infos.data_address;

	for (uint16_t h = 0; h < bitmap_infos.height;
	     h++,
	     bitmap_cursor += bitmap_infos.stride)
	{
		uint8_t * __restrict const start = dst;

		dst = add_padding_left(dst);

		for (uint_fast16_t p = 0; p < bitmap_infos.width; p++)
			dst[p] = bitmap_cursor[p];

		add_padding_right(dst+bitmap_infos.width);
		dst = start + dst_width;
	}
}

/* The dumb way */
static uint_fast32_t ceil_to_power_of_2(uint_fast32_t const value)
{
	uint_fast32_t const values[] = {
		1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096};
	uint_fast32_t next_p2 = 1;

	if (value > 4096) {
		next_p2 = 4096;
		goto out;
	}

	for (uint_fast32_t i = 0;
	     i < sizeof(values)/sizeof(uint_fast32_t);
	     i++)
	{
		uint_fast32_t const p2 = values[i];
		if (value <= p2) {
			next_p2 = p2;
			break;
		}
	}

out:
	return next_p2;
}

static void normalize_coordinates(
	myy_vector_glyph_metadata * __restrict const glyphs_metadata,
	uint_fast32_t texture_width,
	uint_fast32_t texture_height)
{
	myy_vector_for_each_ptr(
		struct myy_packed_fonts_glyphdata, glyph_meta, in, glyphs_metadata,
		{
			normalize_tex_coordinates(glyph_meta,
				texture_width,
				texture_height);
		}
	);
}

static bool write_bitmap(
	int output_fd,
	uint8_t const * __restrict const texture_data,
	uint32_t width,
	uint32_t height)
{
	struct myy_raw_texture_header header = {
		.signature = MYYT_SIGNATURE,
		.width     = width,
		.height    = height,
		.gl_target = GL_TEXTURE_2D,
		.gl_format = GL_ALPHA,
		.gl_type   = GL_UNSIGNED_BYTE,
		.alignment = 4,
		.reserved  = 0
	};

	int64_t written = write(output_fd, &header, sizeof(header));

	if (written != sizeof(header))
		goto could_not_write_header;

	written = write(output_fd, texture_data, width * height);
	if (written != (width * height))
		goto could_not_write_texture;

	sync();

	return true;

could_not_write_texture:
could_not_write_header:
	perror("Could not write the entire texture !\n");
	return false;
}

static bool generate_bitmap(
	myy_vector_bitmap_metadata * __restrict const bitmaps_metadata,
	uint8_t * __restrict texture,
	uint32_t const total_height,
	uint32_t const max_height,
	myy_vector_glyph_metadata * __restrict const glyphs_vector,
	int output_fd)
{
	struct myy_packed_fonts_glyphdata * __restrict const glyph_gldata =
		myy_vector_glyph_metadata_data(glyphs_vector);
	uint32_t const padding = 1;
	uint32_t const sides_padding = padding * 2;

	/* TODO
	 * This completely ignores the maximal padded width
	 * of any character
	 * If the maximum is 37 for example, you wouldn't
	 * be able to fit a single character into one column.
	 */
	uint32_t const column_min_width = 32; /* px */

	uint32_t const n_columns =
		(max_height + total_height + sides_padding) / 4096;
	uint32_t const total_width = column_min_width << (n_columns);

	uint32_t h = 0;
	uint32_t current_line_width = 0;
	uint32_t current_line_max_height = 0;

	uint8_t * __restrict const texture_start = texture;

	texture = add_lower_padding(texture, total_width);
	h += padding;

	myy_vector_for_each(bitmaps_metadata,
		struct bitmap_metadata, metadata,
		{
			uint32_t const padded_width =
				metadata.width + sides_padding;
			uint32_t const added_width =
				current_line_width + padded_width;
			struct myy_packed_fonts_glyphdata * __restrict const
				current_glyph = glyph_gldata+metadata.index;
			if (added_width < total_width)
			{
				blit(texture, total_width, metadata);
				printf("Tex:\n%d ←→ %d\n%d ↑↓ %d\n",
					current_line_width + 1,
					current_line_width + metadata.width,
					h, h+metadata.height-1);
				current_glyph->tex_left   = 1 + current_line_width;
				current_glyph->tex_right  =
					1 + current_line_width + metadata.width;
				current_glyph->tex_bottom = h;
				current_glyph->tex_top    = h+metadata.height;
				current_line_max_height =
					max(current_line_max_height, metadata.height);
				current_line_width = added_width;
				texture += padded_width;
			}
			else if (metadata.width <= total_width)
			{
				h += current_line_max_height + sides_padding;
				texture = texture_start + (h * total_width);
				blit(texture, total_width, metadata);
				current_glyph->tex_left   = 1;
				current_glyph->tex_right  = metadata.width + 1;
				current_glyph->tex_bottom = h;
				current_glyph->tex_top    = h+metadata.height;
				texture += padded_width;
				current_line_width = padded_width;
				current_line_max_height = metadata.height;
			}
		}
	);

	h += current_line_max_height + padding;

	/* The "texture" is a prepared 4K * 4K texture.
	 * It's already as its maximum AND every pixel is set 0
	 * before hand.
	 * So we don't need to pad with "0" again.
	 */
	uint_fast32_t const power_of_2_h = ceil_to_power_of_2(h);

	//normalize_coordinates(glyphs_vector, total_width, power_of_2_h);
	uint32_t const n_pixels = h * total_width;
	uint16_t const mask = total_width - 1;
	for (uint_fast32_t p = 0; p < n_pixels; p++)
	{
		if ((p & mask) == mask) LOG("\n");
		print_pixel(texture_start[p]);
	}

	LOG("Written !\n");

	LOG("total_width : %u\ntotal_height : %lu (%u)\n",
		total_width,
		power_of_2_h,
		h);

	return 
		write_bitmap(output_fd, texture_start, total_width, power_of_2_h);
}

#define write_or_bailout(written, fd, data, data_size, label) { \
	written = write(fd, data, data_size);\
	if (written != data_size)\
		goto label;\
}
bool generate_metadata_file(
	int output_file_fd,
	char const * __restrict const texture_filename,
	myy_vector_u32 * __restrict const codepoints,
	myy_vector_glyph_metadata * __restrict const glyphs,
	int16_t const min_bearing_y)
{

	uint16_t const padding[16] = {0};
	uint32_t const texture_filename_size = strlen(texture_filename);
	LOG("texture_filename_size : %d\n", texture_filename_size);
	struct myy_packed_fonts_textures_filename filename = {
		.size = texture_filename_size,
	};
	uint_fast8_t const texture_filename_padding =
		filename.size - texture_filename_size;

	struct myy_packed_fonts_textures_filenames_section const
		filenames_section_header =
	{
		.n_filenames = 1
	};

	struct myy_packed_fonts_info_header header = {
		.signature = MYYF_SIGNATURE,
		.n_stored_codepoints =
			myy_vector_u32_length(codepoints),
		.codepoints_start_offset  = 0,
		.glyphdata_start_offset   = 0,
		.texture_filenames_offset = 0,
		.min_bearing_y = min_bearing_y,
		.padding = 0,
		.unused = 0
	};

	LOG("Writing the header\n");
	/* Write the dummy header */
	uint32_t offset = 0;
	int written = write(output_file_fd, &header, sizeof(header));
	if (written != sizeof(header))
		goto bail_out;
	offset += written;

	header.texture_filenames_offset = offset;
	/* Write the filenames section */
	{
		uint32_t texture_filename_size_added_padding;
		uint32_t section_padding;
		uint32_t offset_after_texture_filename;
		write_or_bailout(written,
			output_file_fd,
			&filenames_section_header,
			sizeof(filenames_section_header),
			bail_out);
		offset += written;

		/* The "size" needs to be rewritten as
		 * section_size
		 * or something like this.
		 * If you need the string size, just throw strlen on it
		 * or count characters until you hit 0.
		 * The size is here to move the cursor to the next string.
		 */
		offset_after_texture_filename =
			offset + sizeof(filename) + texture_filename_size;
		texture_filename_size_added_padding =
			ALIGN_ON_POW2(offset_after_texture_filename, 8)
			- offset_after_texture_filename;
		filename.size += texture_filename_size_added_padding;

		write_or_bailout(written, 
			output_file_fd,
			&filename,
			sizeof(filename),
			bail_out);
		offset += written;
		
		write_or_bailout(written,
			output_file_fd,
			texture_filename,
			texture_filename_size,
			bail_out);
		offset += written;

		write_or_bailout(written,
			output_file_fd,
			padding,
			texture_filename_size_added_padding,
			bail_out);
		offset += written;

		section_padding = ALIGN_ON_POW2(offset, 16) - offset;
		write_or_bailout(written,
			output_file_fd,
			padding,
			section_padding,
			bail_out);
		offset += written;
	}
	LOG("Filenames written\n");
	header.codepoints_start_offset = offset;

	/* Write the codepoints section */
	{
		/* filename.size is rounded to 8 to avoid unaligned accesses */
		size_t const codepoints_size =
			myy_vector_u32_allocated_used(codepoints);
		size_t codepoints_padding;

		write_or_bailout(written,
			output_file_fd,
			myy_vector_u32_data(codepoints),
			codepoints_size,
			bail_out);
		offset += written;

		LOG("Written the codepoints\n");

		codepoints_padding =
			ALIGN_ON_POW2(offset, 16) - offset;
		write_or_bailout(written,
			output_file_fd,
			padding,
			codepoints_padding,
			bail_out);
		offset += written;
	}
	LOG("Codepoints written\n");
	header.glyphdata_start_offset = offset;

	/* Write the glyphs metadata section */
	{
		size_t const glyphs_size =
			myy_vector_glyph_metadata_allocated_used(glyphs);
		size_t glyphs_padding;

		write_or_bailout(written,
			output_file_fd,
			myy_vector_glyph_metadata_data(glyphs),
			glyphs_size,
			bail_out);
		offset += written;

		glyphs_padding = ALIGN_ON_POW2(offset, 16) - offset;
		write_or_bailout(written,
			output_file_fd,
			padding,
			glyphs_padding,
			bail_out);
		offset += written;
	}
	LOG("Glyphs written\n");
	/* Let's make sure that things won't go wrong when seeking to
	 * the beginning
	 */
	sync();

	/* Write the correct header this time */
	lseek(output_file_fd, 0, SEEK_SET);
	written = write(output_file_fd, &header, sizeof(header));
	if (written != sizeof(header))
		goto bail_out;

	/* Sync one more time, now that all the data are written */
	sync();
	return true;

bail_out:
	perror("Could not write all the metadata\n");
	return false;
}

#define BITMAP_AVERAGE_SIZE (20*25)
/* TODO :
 * - Generate the big bitmap by packing as much elements on a
 *   single line.
 *   The whole logic is :
 *   - Pack all the characters with no width or no height into
 *     the same place.
 *   - Pack all characters with a width below 8 together
 *   - Pack the remaining characters, taking a character and
 *     packing it with a the character that will mostly fill
 *     the remaining space.
 */
int main(int const argc, char const * const * const argv)
{
	enum program_status ret = status_ok;
	int n_fonts;
	char const * const * __restrict fonts_filepath;
	char const * __restrict chars_filepath;
	int output_bitmap_fd;
	int output_metadata_fd;

	bool bool_ret;
	FT_Library library;
	myy_vector_ft_face faces = myy_vector_ft_face_init(16);
	myy_vector_bitmap_metadata bitmaps_metadata =
		myy_vector_bitmap_metadata_init(1024);
	myy_vector_glyph_metadata glyph_metadata =
		myy_vector_glyph_metadata_init(1024);
	myy_vector_u8 bitmaps =
		myy_vector_u8_init(1024*BITMAP_AVERAGE_SIZE);
	myy_vector_u32 codepoints = myy_vector_u32_init(1024*1024);
	/* The texture isn't supposed to expand endlessly.
	 * The maximum authorized size is 4Kx4K.
	 * Some OpenGL ES implementations only support 2Kx2K
	 * After that, you'll need another one.
	 */
	uint8_t * __restrict texture =
		(uint8_t * __restrict) malloc(4096*4096);
	memset(texture, 0, 4096*4096);
	char const * __restrict const bitmap_filename =
		"fonts_bitmap.myyraw";

	if (!myy_vector_ft_face_is_valid(&faces))
	{
		ret = status_not_enough_initial_memory_for_faces;
		goto not_enough_initial_memory_for_faces;
	}

	if (!myy_vector_bitmap_metadata_is_valid(&bitmaps_metadata)) 
	{
		ret = status_not_enough_initial_memory_for_bitmaps_metadata;
		goto not_enough_initial_memory_for_bitmaps_metadata;
	}

	if (!myy_vector_glyph_metadata_is_valid(&glyph_metadata))
	{
		// TODO : Change
		ret = status_not_enough_initial_memory_for_bitmaps_metadata;
		goto not_enough_initial_memory_for_glyph_metadata;
	}

	if (!myy_vector_u8_is_valid(&bitmaps))
	{
		ret = status_not_enough_initial_memory_for_bitmaps;
		goto not_enough_initial_memory_for_bitmaps;
	}

	if (!myy_vector_u32_is_valid(&codepoints))
	{
		ret = status_not_enough_initial_memory_for_codepoints;
		goto not_enough_initial_memory_for_codepoints;
	}

	if (argc < 3) {
		ret = status_not_enough_arguments;
		print_usage(argv[0]);
		goto not_enough_arguments;
	}

	n_fonts = argc-2;
	fonts_filepath = argv+1;
	chars_filepath = argv[argc-1];

	printf("Fonts filepath :\n");
	for (uint32_t i = 0; i < n_fonts; i++) {
		printf("\t%s\n", fonts_filepath[i]);
	}

	LOG("Chars filepath : %s\n", chars_filepath);

	for (uint_fast32_t i = 0; i < n_fonts; i++) {
		if (!fh_file_exist(fonts_filepath[i])) {
			LOG("%s not_found\n", fonts_filepath[i]);
			ret = status_font_filepath_does_not_exist;
			goto font_filepath_does_not_exist;
		}
	}

	if (!fh_file_exist(chars_filepath)) {
		ret = status_chars_filepath_does_not_exist;
		goto chars_filepath_does_not_exist;
	}

	/* Check if we can open all the output files before
	 * doing any serious computation
	 */
	output_bitmap_fd = 
		open(bitmap_filename, O_RDWR|O_CREAT, 00666);
	if (output_bitmap_fd < 0) {
		ret = status_could_not_open_bitmap_output_file;
		goto could_not_open_bitmap_output_file;
	}

	output_metadata_fd =
		open("font_pack_meta.dat", O_RDWR|O_CREAT, 00666);
	if (output_metadata_fd < 0) {
		ret = status_could_not_open_metadata_output_file;
		goto could_not_open_metadata_output_file;
	}

	/* Load the codepoints, store them and sort them */
	bool_ret = load_codepoints_from_chars_file(
		&codepoints, chars_filepath);
	if (!bool_ret)
	{
		ret = status_could_not_load_codepoints_from_file;
		goto could_not_load_codepoints_from_file;
	}

	/* TODO Maybe this should be done before... */
	if (!fonts_init(&library, fonts_filepath, n_fonts, &faces))
	{
		ret = status_could_not_initialize_fonts;
		goto could_not_initialize_fonts;
	}

	int16_t min_bearing_y = 0;
	/* Copy each bitmap individually.
	 * TODO Freetype actually provide facilities to copy bitmap.
	 *      Investigate this. This could ease our work tenfold.
	 */
	bool_ret = compute_each_bitmap_individually(
		&faces,
		&codepoints,
		&bitmaps,
		&bitmaps_metadata,
		&glyph_metadata,
		&min_bearing_y);
	if (!bool_ret)
	{
		ret = status_could_not_get_a_bitmap_for_each_character;
		goto could_not_get_a_bitmap_for_each_character;
	}
	//print_bitmaps(&bitmaps_metadata);

	/* Sort the bitmaps by width, in order to align them afterwards.
	 * We don't do any serious packing work here.
	 * We pack the characters from narrow to wide.
	 */
	sort_bitmaps_metadata_by_width(&bitmaps_metadata);

	/* Determine the sum of every character height
	 * and the maximum width
	 * TODO The total height computation doesn't take
	 *      padding into account.
	 */
	struct global_statistics global_metadata =
		bitmaps_statistics(&bitmaps_metadata);

	/* Generate the texture and write it */
	bool_ret = generate_bitmap(
		&bitmaps_metadata,
		texture,
		global_metadata.max_width, global_metadata.total_height,
		&glyph_metadata,
		output_bitmap_fd);
	if (!bool_ret) {
		ret = status_could_not_write_bitmap_file;
		goto could_not_write_bitmap_file;
	}

	/* Generate and write down the metadata file */
	bool_ret = generate_metadata_file(
		output_metadata_fd,
		bitmap_filename,
		&codepoints,
		&glyph_metadata,
		min_bearing_y);
	if (!bool_ret) {
		ret = status_could_not_write_metadata_file;
		goto could_not_write_metadata_file;
	}

	fonts_deinit(&library, &faces);

could_not_write_metadata_file:
could_not_write_bitmap_file:
could_not_get_a_bitmap_for_each_character:
could_not_initialize_fonts:
could_not_load_codepoints_from_file:
	close(output_metadata_fd);
could_not_open_metadata_output_file:
	close(output_bitmap_fd);
could_not_open_bitmap_output_file:
chars_filepath_does_not_exist:
font_filepath_does_not_exist:
not_enough_arguments:
	myy_vector_u32_free_content(codepoints);
not_enough_initial_memory_for_codepoints:
	myy_vector_u8_free_content(bitmaps);
not_enough_initial_memory_for_bitmaps:
	myy_vector_glyph_metadata_free_content(glyph_metadata);
not_enough_initial_memory_for_glyph_metadata:
	myy_vector_bitmap_metadata_free_content(bitmaps_metadata);
not_enough_initial_memory_for_bitmaps_metadata:
	myy_vector_ft_face_free_content(faces);
not_enough_initial_memory_for_faces:
	LOG("%s", status_messages[ret]);
	return ret;
}

