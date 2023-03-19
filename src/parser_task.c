#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
#include <zephyr/net/buf.h>

#include "lib/parser.h"
#include "lib/blep1.h"

LOG_MODULE_REGISTER(parser_task, LOG_LEVEL_DBG);

#define STACKSIZE 1024
#define PRIORITY 7

K_SEM_DEFINE(parser_task_start, 0, 1);

static struct parser *parser = NULL;
static struct k_fifo *rx_queue = NULL;
static struct k_msgq *telegram_queue = NULL;

int parser_task_init(struct k_fifo *input, struct k_msgq *output) {
    parser = parser_init();
    if (parser == NULL) {
        return -1;
    }
    rx_queue = input;
    telegram_queue = output;
    k_sem_give(&parser_task_start);
    return 0;
}

void parser_task(void *, void *, void *) {
    k_sem_take(&parser_task_start, K_FOREVER);
    LOG_INF("Telegram parser started");
    while (true) {
        struct net_buf *telegram_buf = net_buf_get(rx_queue, K_FOREVER);
        LOG_INF("Received telegram buffer, length %d", telegram_buf->len);
        struct telegram *telegram = parse_telegram(parser, telegram_buf);
        if (telegram != NULL) {
            LOG_INF("Received telegram with length: %d", telegram_items_count(telegram));
            telegram_message message = { telegram };

            // Remove any existing message as they are outdated
            telegram_message discard;
            while(k_msgq_get(telegram_queue, &discard, K_NO_WAIT) == 0) {
                telegram_free(discard.telegram);
                LOG_WRN("Telegram overrun, discarding message");
            }

            int ret = k_msgq_put(telegram_queue, &message, K_NO_WAIT);
            if (ret < 0) {
                LOG_ERR("Failed to send message, %d", ret);
                telegram_free(telegram);
            }
        }
        net_buf_unref(telegram_buf);
    }
}

K_THREAD_DEFINE(parser_task_thread, STACKSIZE,
                parser_task, NULL, NULL, NULL,
                PRIORITY, 0, 0);
