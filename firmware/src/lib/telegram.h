#ifndef TELEGRAM_HEADER_H
#define TELEGRAM_HEADER_H

#include <sys/types.h>

#include "openp1.h"

union data_value {
        uint32_t double_long_unsigned;
        uint16_t long_unsigned;
        int16_t long_signed;
        uint8_t date_time[14];
};

struct data_item {
    enum Item item;
    union data_value value;
};

struct data_list {
    struct data_list *next;
    struct data_item item;
};

struct telegram {
    uint8_t *identifier; 
    struct data_list *data_list_head;
    struct data_list *data_list_tail;
};

typedef struct {
    struct telegram *telegram;
} __attribute__((aligned(4))) telegram_message;

struct telegram_data_iterator {
    struct data_list *_pos;
};

uint16_t data_item_size(struct data_item *data_item);

struct telegram * telegram_init();
void telegram_free(struct telegram *telegram);

void telegram_item_iterator_init(struct telegram *telegram, struct telegram_data_iterator *iter);
int telegram_item_append(struct telegram *telegram, struct data_item *data_item);
struct data_item * telegram_item_iterator_next(struct telegram_data_iterator *iter);

int telegram_items_count(struct telegram *telegram);

#endif /* TELEGRAM_HEADER_H */
