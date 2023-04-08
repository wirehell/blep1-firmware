#include "telegram.h"
#include "openp1.h"
#include "common.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(telegram, LOG_LEVEL_DBG);


uint16_t data_item_size(struct data_item *data_item) {
    switch (data_definition_table[data_item->item].format) {
        case DOUBLE_LONG_UNSIGNED_8_3:
            return sizeof(data_item->value.double_long_unsigned);
        case DOUBLE_LONG_UNSIGNED_4_3:
            return sizeof(data_item->value.double_long_unsigned);
        case DATE_TIME_STRING:
            return sizeof(data_item->value.date_time)/sizeof(uint8_t);
        case LONG_SIGNED_3_1:
            return sizeof(data_item->value.long_signed);
        case LONG_UNSIGNED_3_1:
            return sizeof(data_item->value.long_unsigned);
    
    default:
        LOG_ERR("Size of type not known: %d", data_item->item);
    }
    return -1;
}

struct telegram * telegram_init() {
    struct telegram *telegram = common_heap_alloc(sizeof(struct telegram));
    if (telegram == NULL) {
        LOG_ERR("Could not allocate memory");
        return NULL;
    }
    telegram->identifier = NULL;
    telegram->data_list_head = NULL;
    telegram->data_list_tail = NULL;
    return telegram;
}

void telegram_free(struct telegram *telegram) {
    if (telegram->identifier != NULL) {
        common_heap_free(telegram->identifier);
    }
    struct data_list *current = telegram->data_list_head;
    while (current != NULL) {
        struct data_list *next = current->next;
        common_heap_free(current);
        current = next;
    }
    common_heap_free(telegram);
}

int telegram_item_append(struct telegram *telegram, struct data_item *data_item) {
    struct data_list *list = common_heap_alloc(sizeof(struct data_list));
    if (list == NULL) {
        LOG_ERR("Could not allocate memory for list item");
        return -1;
    }
    list->item = *data_item;
    list->next = NULL;

    struct data_list *tail = telegram->data_list_tail; 
    if (tail == NULL) {
        telegram->data_list_head = list;
        telegram->data_list_tail = list;
    } else {
        telegram->data_list_tail->next = list;
        telegram->data_list_tail = list;
    }
    return 0;
}

void telegram_item_iterator_init(struct telegram *telegram, struct telegram_data_iterator *iter) {
    iter->_pos = telegram->data_list_head;
}

struct data_item * telegram_item_iterator_next(struct telegram_data_iterator *iter) {
    if (iter->_pos == NULL) {
        return NULL;
    }
    struct data_item *item = &(iter->_pos->item);
    iter->_pos = iter->_pos->next;
    return item;
}

int telegram_items_count(struct telegram *telegram) {
    int i = 0;
    struct data_list *current = telegram->data_list_head;
    while (current != NULL) {
        current = current->next;
        i++;
    }
    return i;
}
