#ifndef COMMON_HEADER_H
#define COMMON_HEADER_H

#include <stdlib.h>

void * common_heap_alloc(size_t bytes);
void common_heap_free(void *mem);

#endif /* COMMON_HEADER_H */