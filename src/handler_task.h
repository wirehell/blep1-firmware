#ifndef TELEGRAM_HANDLER_TASK_HEADER_H
#define TELEGRAM_HANDLER_TASK_HEADER_H

#include <zephyr/kernel.h>
#include "lib/telegram.h"

typedef void (*update_value_store_fun)(struct data_item *);

int handler_task_init(struct k_msgq *input, update_value_store_fun update_fun);

#endif /* PARSER_TASK_HEADER_H */