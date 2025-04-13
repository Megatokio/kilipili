// Copyright (c) 2024 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C"
{
#endif

extern void* malloc(size_t size);
extern void* calloc(size_t count, size_t size);
extern void* realloc(void* mem, size_t size);
extern void	 free(void* mem);

using dump_heap_print_fu = void(void* data, uint32_t* addr, int sz, int free0_used1_invalid2);

extern const char* check_heap(); // returns nullptr or error text
extern void		   dump_heap();	 // dump to stdout
extern void		   dump_heap_to_fu(dump_heap_print_fu*, void* data);
extern size_t	   heap_total_size();
extern size_t	   heap_largest_free_block();
extern bool		   heap_cut_exception_block(); // if you are desperate for memory


#ifdef __cplusplus
}
#endif
