#ifndef MYY_VECTOR_H
#define MYY_VECTOR_H 1

#include <helpers/log.h>

#ifndef _POSIX_C_SOURCE
#define HAD_TO_DEFINE_POSIX_C_SOURCE 1
#define _POSIX_C_SOURCE 200112L
#endif

#include <stdlib.h>

/* Who knows what could happen with the Android stupid build
 * system if we let those macros on.
 */
#ifdef HAD_TO_DEFINE_POSIX_C_SOURCE
#undef _POSIX_C_SOURCE
#undef HAD_TO_DEFINE_POSIX_C_SOURCE
#endif

#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <stdbool.h>

#define ALIGN_ON_POW2(x, p) ((x)+(p-1) & ~(p-1))
#define MYY_DEFAULT_VECTOR_SIZE (64)

/* Because Android is too fucking stupid to provide
 * a real implementation of aligned_alloc in its stdlib.
 */
void * __attribute__ ((weak)) aligned_alloc(
	size_t const alignment,
	size_t const size)
{
	/* I REALLY hope that they've got a function that
	 * dates from the early 2000
	 */
	void * allocated_space_address = (void *) 0;

	int ret =
		posix_memalign(&allocated_space_address, alignment, size);

	if (ret == 0)
		return (void *) allocated_space_address;
	else
		return (void *) 0;
}

struct myy_vector {
	uintptr_t begin;
	uintptr_t tail;
	uintptr_t end;
};

typedef struct myy_vector myy_vector_t;

static inline bool myy_vector_can_add(
	struct myy_vector const * __restrict const vector,
	size_t const n_octets)
{
	return ((vector->tail + n_octets) < vector->end);
}

static inline bool myy_vector_can_store(
	struct myy_vector const * __restrict const vector,
	size_t const total_octets)
{
	return ((vector->begin + total_octets) < vector->end);
}

static inline size_t myy_vector_allocated_total(
	struct myy_vector const * __restrict const vector)
{
	return (size_t) (vector->end - vector->begin);
}

static inline size_t myy_vector_allocated_used(
	struct myy_vector const * __restrict const vector)
{
	return (size_t) (vector->tail - vector->begin);
}

static inline void myy_vector_reset(
	struct myy_vector * __restrict const vector)
{
	vector->tail = vector->begin;
}

static inline bool myy_vector_expand_to_store_at_least(
	struct myy_vector * const vector,
	size_t const n_octets)
{
	size_t const vector_size     =
		myy_vector_allocated_total(vector);
	size_t const new_vector_size =
		ALIGN_ON_POW2(
			(vector_size + n_octets),
			MYY_DEFAULT_VECTOR_SIZE);
	size_t const vector_last_offset =
		myy_vector_allocated_used(vector);
	uintptr_t new_begin = (uintptr_t) realloc(
		(uint8_t * __restrict) vector->begin,
		new_vector_size);

	bool success = (new_begin != 0);

	if (success) {
		vector->begin = new_begin;
		vector->tail  = new_begin + vector_last_offset;
		vector->end   = new_begin + new_vector_size;
	}

	return success;
}

static inline bool myy_vector_ensure_enough_space_for(
	struct myy_vector * const vector,
	size_t const n_octets)
{
	return 
		myy_vector_can_add(vector, n_octets) ||
		myy_vector_expand_to_store_at_least(vector, n_octets);
}

bool myy_vector_add(
	struct myy_vector * const vector,
	size_t const n_octets,
	uint8_t const * __restrict const source);


struct myy_vector myy_vector_init(
	size_t const n_octets);

static inline void myy_vector_free_content(
	struct myy_vector const vector)
{
	free((uint8_t * __restrict) vector.begin);
}

static inline bool myy_vector_is_valid(
	struct myy_vector const * __restrict const vector)
{
	return (((uint8_t const * __restrict) vector->begin) != NULL);
}

static inline uint8_t * myy_vector_data(
	struct myy_vector const * __restrict const vector)
{
	return (uint8_t * __restrict) (vector->begin);
}

static inline void myy_vector_forget_last(
	struct myy_vector * __restrict const vector,
	size_t const n_octets)
{
	vector->tail -= n_octets;
}

static inline void myy_vector_inspect(
	struct myy_vector * __restrict const vector)
{
	LOG("Begin : 0x%016" PRIxPTR "\n"
		"Tail  : 0x%016" PRIxPTR "\n"
		"End   : 0x%016" PRIxPTR "\n",
		vector->begin, vector->tail, vector->end);
}

static inline bool myy_vector_force_length_to(
	struct myy_vector * __restrict const vector,
	size_t const size)
{
	bool const got_enough_space =
		myy_vector_ensure_enough_space_for(vector, size);
	if (got_enough_space)
		vector->tail = (uintptr_t) (vector->begin + size);
	return got_enough_space;
}

/**
 * Move the all the content, starting from "index", to the new
 * index.
 * 
 * Mainly used to insert some content in the middle of the
 * vector.
 * 
 * @param vector
 * The vector to shift content in
 * 
 * @param from
 * The index where we should start shifting
 * 
 * @param to    
 * The new index where the content will be moved.
 * 
 * @return
 * true if the content was moved.
 * false otherwise.
 */


#define myy_vector_for_each(vector, T, name, ...) {\
	T const * __restrict _cursor =         \
		(T * __restrict) ((vector)->begin);   \
	T const * __restrict const _end =            \
		(T * __restrict) ((vector)->tail);     \
	while(_cursor < _end) {\
		T const name = *_cursor++; \
		__VA_ARGS__\
	}\
}\

#define myy_vector_for_each_ptr(T, name, ignored_word, vector, ...) {\
	T * __restrict _cursor =         \
		(T * __restrict) ((vector)->begin);   \
	T const * __restrict const _end =            \
		(T * __restrict) ((vector)->tail);     \
	while(_cursor < _end) {\
		T * __restrict const name = _cursor; \
		_cursor++; \
		__VA_ARGS__\
	}\
}\

#include "da_c_template.h"

#ifdef __cplusplus

#include "da_cxx_template.h"

#define myy_vector_template(suffix, T) \
myy_vector_common_template(suffix, T) \
myy_vector_c_template(suffix, T) \
myy_vector_cxx_template(suffix, T)

#else

#define myy_vector_template(suffix, T) \
myy_vector_common_template(suffix, T) \
myy_vector_c_template(suffix, T)

#endif


#endif
