#ifndef MYY_HELPERS_MATRICES_H
#define MYY_HELPERS_MATRICES_H 1

#ifdef __clang__
#define myy_cattr_vector(components,total_size) __attribute__ ((ext_vector_type(components)))
#else
#define myy_cattr_vector(components,total_size) __attribute__ ((vector_size(total_size)))
#endif
typedef float vec4 myy_cattr_vector(4,16); // __attribute__ ((vector_size (16)));

struct myy_matrix_4_row { float X, Y, Z, W; };
union myy_4x4_matrix {
        float raw_data[16];
        vec4 vec_rows[4];
        struct {
                struct myy_matrix_4_row x, y, z, w;
        } row;
};
typedef union myy_4x4_matrix myy_4x4_matrix_t;

__attribute__((unused))
static inline void myy_matrix_4x4_ortho_layered_window_coords(
	union myy_4x4_matrix * __restrict const matrix,
	unsigned int const width,
	unsigned int const height,
	unsigned int const layers)
{

	union myy_4x4_matrix const ortho_matrix = {
		.vec_rows = {
			{2.0f/width, 0,            0,           0},
			{0,          -2.0f/height, 0,           0},
			{0,          0,            1.0f/layers, 0},
			{-1.0f,      1.0f,         0,           1.0f}
		}
	};
	*matrix = ortho_matrix;
}

#endif