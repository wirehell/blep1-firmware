#include "common.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#define ALIGNMENT 4

K_HEAP_DEFINE(common_heap, 16384);

void * common_heap_alloc(size_t bytes) {
    return k_heap_aligned_alloc(&common_heap, ALIGNMENT, bytes, K_NO_WAIT);
}

void common_heap_free(void *mem) {
    k_heap_free(&common_heap, mem);
}
