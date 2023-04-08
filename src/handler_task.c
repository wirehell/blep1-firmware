#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
#include <sys/types.h>

#include "handler_task.h"

#include "lib/telegram.h"
#include "lib/openp1.h"

LOG_MODULE_REGISTER(handler_task, LOG_LEVEL_DBG);

#define STACKSIZE 1024
#define PRIORITY 7

K_SEM_DEFINE(handler_task_start, 0, 1);

static struct k_msgq *telegram_queue = NULL;
static update_value_store_fun update_value_store;

int handler_task_init(struct k_msgq *input, update_value_store_fun update_fun) {
    telegram_queue = input;
    update_value_store = update_fun;
    k_sem_give(&handler_task_start);
    return 0;
}

void handle_telegram(struct telegram *telegram) {
    int count = 0;
    struct data_item *data_item;
    struct telegram_data_iterator iter;
    telegram_item_iterator_init(telegram, &iter);
    while (NULL != (data_item = telegram_item_iterator_next(&iter))) {
        update_value_store(data_item);
        count++;
    }
    LOG_DBG("Value store updated with %d values.", count);
}

void handler_task(void *, void *, void *) {
    k_sem_take(&handler_task_start, K_FOREVER);
    LOG_INF("Telegram handler started");
    while (true) {
        telegram_message telegram_message;
        int ret = k_msgq_get(telegram_queue, &telegram_message, K_FOREVER);
        if (ret < 0) {
            LOG_ERR("Failed to receive message for some reason: %d", ret);
        } else {
            struct telegram *telegram = telegram_message.telegram;
            LOG_INF("Handler task received telegram");
            handle_telegram(telegram);
            telegram_free(telegram);
        }
    }
}

K_THREAD_DEFINE(handler_task_thread, STACKSIZE,
                handler_task, NULL, NULL, NULL,
                PRIORITY, 0, 0);
