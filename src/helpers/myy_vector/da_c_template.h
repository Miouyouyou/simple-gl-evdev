#ifndef MYY_HELPERS_DA_C_TEMPLATE_H
#define MYY_HELPERS_DA_C_TEMPLATE_H 1

#define myy_vector_common_template(suffix, T)                           \
                                                                        \
	struct myy_vector_##suffix##_st {                                   \
		uintptr_t begin;                                                \
		uintptr_t tail;                                                 \
		uintptr_t end;                                                  \
	};                                                                  \
                                                                        \
	typedef struct myy_vector_##suffix##_st myy_vector_##suffix;        \
                                                                        \
	__attribute__((unused))                                             \
	static inline myy_vector_##suffix  myy_vector_##suffix##_init(      \
		size_t n_elements)                                              \
	{                                                                   \
		myy_vector_##suffix vector;                                     \
                                                                        \
		size_t const asked_size =                                       \
			ALIGN_ON_POW2(n_elements * sizeof(T), 64);                  \
		size_t const allocated_size =                                   \
			(asked_size > 64 ? asked_size : 64);                        \
		LOG("Allocated_size : %zu\n", allocated_size);                  \
		uintptr_t const begin =                                         \
			(uintptr_t) (aligned_alloc(64, allocated_size));            \
		memset((void *) begin, 0, allocated_size);                      \
		vector.begin = begin;                                           \
		vector.tail  = begin;                                           \
		vector.end   = begin + allocated_size;                          \
                                                                        \
		return vector;                                                  \
	}                                                                   \
  

#define myy_vector_c_template(suffix, T)                                           \
                                                                                   \
	__attribute__((unused))                                                        \
	static inline bool myy_vector_##suffix##_can_add(                              \
		myy_vector_##suffix const * __restrict const vector,                       \
		size_t const n_elements)                                                   \
	{                                                                              \
		return myy_vector_can_add(                                                 \
			(struct myy_vector const *) vector,                                    \
			n_elements * sizeof(T));                                               \
	}                                                                              \
                                                                                   \
    __attribute__((unused))                                                        \
	static inline bool myy_vector_##suffix##_can_store(                            \
		myy_vector_##suffix const * __restrict const vector,                       \
		size_t const total_elements)                                               \
	{                                                                              \
		return myy_vector_can_store(                                               \
			(struct myy_vector const *) vector,                                    \
			total_elements * sizeof(T));                                           \
	}                                                                              \
                                                                                   \
	__attribute__((unused))                                                        \
	static inline T * myy_vector_##suffix##_data(                                  \
		myy_vector_##suffix  * __restrict const vector)                            \
	{                                                                              \
		return (T *) (myy_vector_data((struct myy_vector *) vector));              \
	}                                                                              \
                                                                                   \
	__attribute__((unused))                                                        \
	static inline size_t myy_vector_##suffix##_allocated_total(                    \
		myy_vector_##suffix  const * __restrict const vector)                      \
	{                                                                              \
		return (size_t) (                                                          \
			myy_vector_allocated_total((struct myy_vector const *) vector)         \
		);                                                                         \
	}                                                                              \
                                                                                   \
	__attribute__((unused))                                                        \
	static inline size_t myy_vector_##suffix##_allocated_used(                     \
		myy_vector_##suffix  const * __restrict const vector)                      \
	{                                                                              \
		return (size_t) (                                                          \
			myy_vector_allocated_used((struct myy_vector const *) vector)          \
		);                                                                         \
	}                                                                              \
                                                                                   \
	__attribute__((unused))                                                        \
	static inline size_t myy_vector_##suffix##_length(                             \
		myy_vector_##suffix  const * __restrict const vector)                      \
	{                                                                              \
		return                                                                     \
			myy_vector_allocated_used((struct myy_vector *) vector)                \
			/ sizeof(T);                                                           \
	}                                                                              \
                                                                                   \
	__attribute__((unused))                                                        \
	static inline bool myy_vector_##suffix##_expand_to_store_at_least(             \
		myy_vector_##suffix  * __restrict const vector,                            \
		size_t const n_elements)                                                   \
	{                                                                              \
		return myy_vector_expand_to_store_at_least(                                \
			((struct myy_vector *) vector),                                        \
			n_elements * sizeof(T));                                               \
	}                                                                              \
                                                                                   \
	__attribute__((unused))                                                        \
	static inline bool myy_vector_##suffix##_ensure_enough_space_for(              \
		myy_vector_##suffix * __restrict const vector,                             \
		size_t const n_elements)                                                   \
	{                                                                              \
		return myy_vector_ensure_enough_space_for(                                 \
			((struct myy_vector *) vector),                                        \
			n_elements * sizeof(T));                                               \
	}                                                                              \
                                                                                   \
	__attribute__((unused))                                                        \
	static inline bool myy_vector_##suffix##_add(                                  \
		myy_vector_##suffix  * __restrict const vector,                            \
		size_t const n_elements,                                                   \
		T const * __restrict const source)                                         \
	{                                                                              \
		return myy_vector_add(                                                     \
			(struct myy_vector *) vector,                                          \
			n_elements * sizeof(T),                                                \
			(uint8_t const *) source);                                             \
	}                                                                              \
                                                                                   \
	__attribute__((unused))                                                        \
	static inline bool myy_vector_##suffix##_add_empty(                            \
		myy_vector_##suffix * __restrict const vector,                             \
		size_t const n_elements)                                                   \
	{                                                                              \
		bool const enough_space =                                                  \
			myy_vector_##suffix##_ensure_enough_space_for(vector, n_elements);     \
		if (enough_space) {                                                        \
			vector->tail += (n_elements * sizeof(T));                              \
		}                                                                          \
		return enough_space;                                                       \
	}                                                                              \
                                                                                   \
                                                                                   \
                                                                                   \
	__attribute__((unused))                                                        \
	static inline void myy_vector_##suffix##_free_content(                         \
		myy_vector_##suffix  const vector)                                         \
	{                                                                              \
		free((T*) (vector.begin));                                                 \
	}                                                                              \
                                                                                   \
	__attribute__((unused))                                                        \
	static inline bool myy_vector_##suffix##_is_valid(                             \
		myy_vector_##suffix  const * __restrict const vector)                      \
	{                                                                              \
		return myy_vector_is_valid((struct myy_vector const *) vector);            \
	}                                                                              \
                                                                                   \
	__attribute__((unused))                                                        \
	static inline void myy_vector_##suffix##_forget_last(                          \
		myy_vector_##suffix  * __restrict const vector,                            \
		size_t const n_elements)                                                   \
	{                                                                              \
		myy_vector_forget_last(                                                    \
			(struct myy_vector *) vector,                                          \
			(n_elements * sizeof(T)));                                             \
	}                                                                              \
                                                                                   \
	__attribute__((unused))                                                        \
	static inline void myy_vector_##suffix##_inspect(                              \
		myy_vector_##suffix  * __restrict const vector)                            \
	{                                                                              \
		LOG(                                                                       \
			"Begin            : 0x%016" PRIxPTR "\n"                               \
			"Tail             : 0x%016" PRIxPTR "\n"                               \
			"End              : 0x%016" PRIxPTR "\n"                               \
			"N Elements       : %zu\n"                                             \
			"Allocated memory : %zu\n"                                             \
			"Used memory      : %zu\n",                                            \
			vector->begin, vector->tail, vector->end,                              \
			myy_vector_##suffix##_length(vector),                                  \
			myy_vector_##suffix##_allocated_total(vector),                         \
			myy_vector_##suffix##_allocated_used(vector));                         \
	}                                                                              \
                                                                                   \
	__attribute__((unused))                                                        \
	static inline T myy_vector_##suffix##_at(                                      \
		myy_vector_##suffix  * __restrict const vector,                            \
		size_t const index)                                                        \
	{                                                                              \
		return myy_vector_##suffix##_data(vector)[index];                          \
	}                                                                              \
                                                                                   \
	__attribute__((unused))                                                        \
	static inline T * myy_vector_##suffix##_at_ptr(                                \
		myy_vector_##suffix  * __restrict const vector,                            \
		size_t const index)                                                        \
	{                                                                              \
		return (myy_vector_##suffix##_data(vector)) + index;                       \
	}                                                                              \
                                                                                   \
	__attribute__((unused))                                                        \
	static inline size_t myy_vector_##suffix##_type_size()                         \
	{                                                                              \
		return sizeof(T);                                                          \
	}                                                                              \
	                                                                               \
	__attribute__((unused))                                                        \
	static inline void myy_vector_##suffix##_reset(                                \
		myy_vector_##suffix * __restrict const vector)                             \
	{                                                                              \
		return myy_vector_reset((struct myy_vector *) vector);                     \
	}                                                                              \
                                                                                   \
	__attribute__((unused))                                                        \
	static inline bool myy_vector_##suffix##_shift_from(                           \
		myy_vector_##suffix * __restrict const vector,                             \
		size_t const from,                                                         \
		size_t const to)                                                           \
	{                                                                              \
		/* TODO Can be simplified with "can_store" */                              \
		/* Hmm...                                            */                    \
		/* Can't use myy_vector_T_can_add that easily since  */                    \
		/* "size_t" is unsigned and "to - from" will give    */                    \
		/* absurd unsigned results if "to" is before "from", */                    \
		/* and fail myy_vector_T_can_add even though we      */                    \
		/* CLEARLY have enough space in that case.           */                    \
                                                                                   \
		if (to > from                                                              \
		    && !myy_vector_##suffix##_can_add(vector, to - from)                   \
			&& !myy_vector_##suffix##_expand_to_store_at_least(vector, to - from)) \
			return false;                                                          \
                                                                                   \
                                                                                   \
		T * const from_address = myy_vector_##suffix##_at_ptr(vector, from);       \
		T * const to_address   = myy_vector_##suffix##_at_ptr(vector, to);         \
		size_t const n_bytes   =                                                   \
			(size_t)                                                               \
			( vector->tail - ((uintptr_t) from_address) );                         \
                                                                                   \
		memmove(to_address, from_address, n_bytes);                                \
                                                                                   \
		/* Readjust the end of the vector */                                       \
		vector->tail = ((uintptr_t) to_address) + n_bytes;                         \
                                                                                   \
		return true;                                                               \
	}                                                                              \
                                                                                   \
	__attribute__((unused))                                                        \
	static inline bool myy_vector_##suffix##_write_at(                             \
		myy_vector_##suffix * __restrict const vector,                             \
		size_t const from,                                                         \
		T const * __restrict const new_elements,                                   \
		size_t const n_new_elements)                                               \
	{                                                                              \
		size_t const total = from + n_new_elements;                                \
		bool can_write =                                                           \
			myy_vector_##suffix##_can_store(vector, total)                         \
			|| myy_vector_##suffix##_expand_to_store_at_least(vector, total);      \
                                                                                   \
		if (can_write)                                                             \
			memcpy(                                                                \
				myy_vector_##suffix##_at_ptr(vector, from),                        \
				new_elements,                                                      \
				n_new_elements * sizeof(T));                                       \
                                                                                   \
		return can_write;                                                          \
	}                                                                              \
                                                                                   \
	__attribute__((unused))                                                        \
	static inline bool myy_vector_##suffix##_delete(                               \
		myy_vector_##suffix * __restrict const vector,                             \
		size_t const index)                                                        \
	{                                                                              \
		return myy_vector_##suffix##_shift_from(vector, index+1, index);           \
	}                                                                              \
	__attribute__((unused))                                                        \
	static inline T * myy_vector_##suffix##_last(                                  \
		myy_vector_##suffix * __restrict const vector)                             \
	{                                                                              \
		size_t const length =                                                      \
			myy_vector_##suffix##_length(vector);                                  \
		if (length > 0) {                                                          \
			return                                                                 \
				myy_vector_##suffix##_at_ptr(vector, length - 1);                  \
		}                                                                          \
		else return NULL;                                                          \
	}                                                                              \
	                                                                               \
	__attribute__((unused))                                                        \
	static inline T * myy_vector_##suffix##_tail_ptr(                              \
		myy_vector_##suffix * __restrict const vector)                             \
	{                                                                              \
		size_t const length =                                                      \
			myy_vector_##suffix##_length(vector);                                  \
		return                                                                     \
			myy_vector_##suffix##_at_ptr(vector, length);                          \
	}                                                                              \
	__attribute__((unused))                                                        \
	static inline bool myy_vector_##suffix##_delete_if(                            \
		myy_vector_##suffix * __restrict const vector,                             \
		T const * __restrict const b,                                              \
		bool (*predicate)(                                                         \
			T const * __restrict const a,                                          \
			T const * __restrict const b))                                         \
	{                                                                              \
		size_t current_length =                                                    \
			myy_vector_##suffix##_length(vector);                                  \
		T * __restrict const data = myy_vector_##suffix##_data(vector);            \
		bool deleted_any = false;                                                  \
		for (size_t i = 0; i < current_length; i++) {                              \
			T * __restrict const a = data+i;                                       \
			bool const should_delete = predicate(a,b);                             \
			deleted_any |= should_delete;                                          \
			if (should_delete) {                                                   \
				myy_vector_##suffix##_delete(vector, i);                           \
				i--; /* Restart from the same index on the next iteration */       \
				current_length--;                                                  \
				deleted_any = true;                                                \
			}                                                                      \
			                                                                       \
		}                                                                          \
		return deleted_any;                                                        \
	}                                                                              \
                                                                                   \
	__attribute__((unused))                                                        \
	static inline void myy_vector_##suffix##_for_each(                             \
		myy_vector_##suffix * __restrict const vector,                             \
		void (*action)(T *))                                                       \
	{                                                                              \
		T * __restrict cursor =                                                    \
			myy_vector_##suffix##_data(vector);                                    \
		T const * __restrict const end =                                           \
			(T const * __restrict) (vector->tail);                                 \
		while (cursor < end) {                                                     \
			action(cursor);                                                        \
			cursor++;                                                              \
		}                                                                          \
	}                                                                              \
	                                                                               \
	__attribute__((unused))                                                        \
	static inline bool myy_vector_##suffix##_force_length_to(                      \
		myy_vector_##suffix * __restrict const vector,                             \
		size_t const n_elements)                                                   \
	{                                                                              \
		return myy_vector_force_length_to(                                         \
			(struct myy_vector *) vector,                                          \
			n_elements * sizeof(T));                                               \
	}                                                                              \

#endif
