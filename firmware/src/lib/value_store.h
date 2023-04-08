#ifndef VALUE_STORE_H
#define VALUE_STORE_H

#include "telegram.h"
#include "openp1.h"

#define BEST_BEFORE_MS 60 * 1000 

struct value_store_row {
    struct data_item data;
	uint64_t last_updated;
};

struct value_store {
    struct value_store_row rows[_ITEM_COUNT];
};

enum value_store_read_status {
    OK,
    STALE,
    INVALID,
};

struct value_store_read_result {
    enum value_store_read_status status;
    union {
        struct data_item data;
        int undefined;
    } data;
};

int value_store_init(struct value_store *store);
void value_store_update(struct value_store *store, struct data_item *data);
struct value_store_read_result value_store_read(struct value_store *store, uint16_t item);
int value_store_copy(struct value_store *src, struct value_store *dst);

#endif /* VALUE_STORE_H */