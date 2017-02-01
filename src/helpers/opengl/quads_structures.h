#ifndef MYY_SRC_HELPERS_OPENGL_QUADS_STRUCTURES
#define MYY_SRC_HELPERS_OPENGL_QUADS_STRUCTURES 1

#include <myy/current/opengl.h>
#include <myy/helpers/struct.h>
#include <stdint.h>

struct point_2D { GLfloat x, y; } __PALIGN__;
struct point_3D { GLfloat x, y, z; } __PALIGN__;
struct triangle_2D { struct point_2D a, b, c; } __PALIGN__;
struct layered_triangle_2D { struct point_3D a, b, c; } __PALIGN__;
struct two_triangles_quad_2D {
	struct triangle_2D first, second;
} __PALIGN__;
struct two_layered_triangles_quad_2D {
	struct layered_triangle_2D first, second;
} __PALIGN__;

struct textured_point_2D { GLfloat x, y, s, t; } __PALIGN__;
struct textured_point_3D { GLfloat x, y, z, s, t; } __PALIGN__;
struct textured_triangle_2D {
	struct textured_point_2D a, b, c;
} __PALIGN__;
struct textured_layered_triangle_2D {
	struct textured_point_3D a, b, c;
} __PALIGN__;
struct two_triangles_textured_quad_2D {
	struct textured_triangle_2D first, second;
} __PALIGN__;
struct two_textured_layered_triangles_quad {
	struct textured_layered_triangle_2D first, second;
} __PALIGN__;

union two_triangles_textured_quad_2D_representations {
	struct two_triangles_textured_quad_2D quad;
	struct textured_triangle_2D triangles[2];
	struct textured_point_2D points[6];
	GLfloat raw_coords[24];
} __PALIGN__;

union two_textured_layered_triangles_quad_representations {
	struct two_textured_layered_triangles_quad quad;
	struct textured_layered_triangle_2D triangles[2];
	struct textured_point_3D points[6];
	GLfloat raw_coords[30];
} __PALIGN__;

typedef union two_triangles_textured_quad_2D_representations two_tris_quad;
typedef union two_textured_layered_triangles_quad_representations two_layered_tris_quad;

#define TWO_TRIANGLES_TEX_QUAD(left, right, down, up, left_tex, right_tex, down_tex, up_tex) { \
	.points = { \
    { .x = left,  .y = up,   .s = left_tex,  .t = up_tex},   \
    { .x = left,  .y = down, .s = left_tex,  .t = down_tex}, \
    { .x = right, .y = up,   .s = right_tex, .t = up_tex},   \
    { .x = right, .y = down, .s = right_tex, .t = down_tex}, \
    { .x = right, .y = up,   .s = right_tex, .t = up_tex},   \
    { .x = left,  .y = down, .s = left_tex,  .t = down_tex}  \
	} \
}

struct byte_point_2D { GLbyte x, y; } __ALIGN_AT(2);
struct BUS_textured_point_2D { GLushort s, t; GLbyte x, y; } __PALIGN__;
struct BUS_textured_triangle_2D { struct BUS_textured_point_2D a, b, c; } __PALIGN__;
struct BUS_two_triangles_textured_quad_2D { struct BUS_textured_triangle_2D first, second; } __PALIGN__;

union BUS_two_triangles_textured_quad_2D_representations {
	struct BUS_two_triangles_textured_quad_2D quad;
	struct BUS_textured_triangle_2D triangles[2];
	struct BUS_textured_point_2D points[6];
} __PALIGN__;

typedef union BUS_two_triangles_textured_quad_2D_representations BUS_two_tris_quad;

#define TWO_BYTES_TRIANGLES_TEX_QUAD(left, right, down, up, left_tex, right_tex, down_tex, up_tex) { \
	.points = { \
    { .s = left_tex,  .t = up_tex,   .x = left,  .y = up,  },  \
    { .s = left_tex,  .t = down_tex, .x = left,  .y = down },  \
    { .s = right_tex, .t = up_tex,   .x = right, .y = up,  },  \
    { .s = right_tex, .t = down_tex, .x = right, .y = down },  \
    { .s = right_tex, .t = up_tex,   .x = right, .y = up,  },  \
    { .s = left_tex,  .t = down_tex, .x = left,  .y = down },  \
	} \
}

#define STXYZ_QUAD(left, right, down, up, layer, left_tex, right_tex, down_tex, up_tex) { \
	.points = { \
    { .s = left_tex,  .t = up_tex,   .x = left,  .y = up,   .z = layer },  \
    { .s = left_tex,  .t = down_tex, .x = left,  .y = down, .z = layer },  \
    { .s = right_tex, .t = up_tex,   .x = right, .y = up,   .z = layer },  \
    { .s = right_tex, .t = down_tex, .x = right, .y = down, .z = layer },  \
    { .s = right_tex, .t = up_tex,   .x = right, .y = up,   .z = layer },  \
    { .s = left_tex,  .t = down_tex, .x = left,  .y = down, .z = layer },  \
	} \
}

struct byte_point_3D { GLbyte x, y, z; } __PALIGN__;
struct BUS_textured_point_3D { GLushort s, t; GLbyte x, y, z; } __PALIGN__;
struct BUS_textured_triangle_3D { struct BUS_textured_point_3D a, b, c; } __PALIGN__;
struct BUS_two_triangles_textured_quad_3D { struct BUS_textured_triangle_3D first, second; } __PALIGN__;

union BUS_two_triangles_textured_quad_3D_representations {
	struct BUS_two_triangles_textured_quad_3D quad;
	struct BUS_textured_triangle_3D triangles[2];
	struct BUS_textured_point_3D points[6];
} __PALIGN__;

typedef union BUS_two_triangles_textured_quad_3D_representations BUS_two_tris_3D_quad;

struct US_textured_point_2D { GLushort s, t; GLshort x, y; } __PALIGN__;
struct US_textured_triangle_2D { struct US_textured_point_2D a, b, c; } __PALIGN__;
struct US_two_triangles_textured_quad_2D { struct US_textured_triangle_2D first, second; } __PALIGN__;

union US_two_triangles_textured_quad_2D_representations {
	struct US_two_triangles_textured_quad_2D quad;
	struct US_textured_triangle_2D triangles[2];
	struct US_textured_point_2D points[6];
} __PALIGN__;

struct US_textured_point_3D { GLushort s, t; GLshort x, y, z; } __PALIGN__;
struct US_textured_triangle_3D { struct US_textured_point_3D a, b, c; } __PALIGN__;
struct US_two_triangles_textured_quad_3D { struct US_textured_triangle_3D first, second; } __PALIGN__;

union US_two_triangles_textured_quad_3D_representations {
	struct US_two_triangles_textured_quad_3D quad;
	struct US_textured_triangle_3D triangles[2];
	struct US_textured_point_3D points[6];
} __PALIGN__;

typedef union US_two_triangles_textured_quad_2D_representations US_two_tris_quad;
typedef union US_two_triangles_textured_quad_3D_representations US_two_tris_quad_3D;

enum quad_coords_order {
	upleft_corner, downleft_corner, upright_corner, downright_corner,
	repeated_upright_corner, repeated_downleft_corner, two_triangles_corners
};


#endif
