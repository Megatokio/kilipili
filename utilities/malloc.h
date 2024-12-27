// Copyright (c) 2024 - 2024 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include <cstdlib>

#ifdef __cplusplus
extern "C"
{
#endif

extern void* malloc(size_t size);
extern void* calloc(size_t count, size_t size);
extern void* realloc(void* mem, size_t size);
extern void	 free(void* mem);

extern const char* check_heap(); // returns nullptr or error text
extern void		   dump_heap();	 // dump to stdout

#ifdef __cplusplus
}
#endif
