
#include "value_store.h"
#include "blep1.h"
#include "stdint.h"

#include <zephyr/kernel.h>

int value_store_init(struct value_store *store) {
    for(int i = 0 ; i < _ITEM_COUNT ; i++) {
        store->rows[i].last_updated = -1;
    }
    return 0;
}

void value_store_update(struct value_store *store, struct data_item *data) {
    // Todo add mutex against copy
    store->rows[data->item].data = *data;
    store->rows[data->item].last_updated = k_uptime_get();
}

struct value_store_read_result value_store_read(struct value_store *store, uint16_t item) {
    struct value_store_read_result result;
    if (item >= _ITEM_COUNT) {
        result.status = INVALID;
        return result;
    }

    struct value_store_row *row = &store->rows[item];

    uint64_t current_time = k_uptime_get();
    if (current_time > row->last_updated + BEST_BEFORE_MS) {
        result.status = STALE;
        return result;
    }

    result.data.data = row->data;
    result.status = OK;
    return result;
}

int value_store_copy(struct value_store *src, struct value_store *dst) {
    *dst = *src;
    return 0;
}

