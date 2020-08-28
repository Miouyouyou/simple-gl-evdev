#include <malloc.h>
#include <stdint.h>
#include <string.h>

#include <helpers/myy_vector/vector.h>

bool myy_vector_add(
	struct myy_vector * const vector,
	size_t const n_octets,
	uint8_t const * __restrict const source)
{
	bool can_add = myy_vector_can_add(vector, n_octets);
	bool added = false;

	if (can_add ||
	    myy_vector_expand_to_store_at_least(vector, n_octets))
	{
		memcpy(
			(uint8_t * __restrict) vector->tail,
			source,
			n_octets);
		vector->tail += n_octets;
		added = true;
	}

	return added;
}

struct myy_vector myy_vector_init(
	size_t const n_octets)
{
	struct myy_vector vector;

	size_t allocated_size = 
		ALIGN_ON_POW2(n_octets, MYY_DEFAULT_VECTOR_SIZE);
	LOG("\n\n\n\n[Myy_vector_init] (%zu octets)\n", allocated_size);

	uintptr_t const begin = (uintptr_t) (malloc(allocated_size));
	memset((void *) begin, 0, allocated_size);
	vector.begin = begin;
	vector.tail  = begin;
	vector.end   = begin + allocated_size;

	return vector;
}
