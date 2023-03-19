#include <zephyr/logging/log.h>
#include <zephyr/net/buf.h>
#include <zephyr/kernel.h>
#include <sys/types.h>

#include "framer_task.h"
#include "lib/blep1.h"
#include "line_log.h"
#include "lib/telegram_framer.h"

#define READ_TIMEOUT_MS 200
#define MAX_TELEGRAM_SIZE 8192
#define TELEGRAM_BUF_POOL_SIZE 4

LOG_MODULE_REGISTER(framer_task, LOG_LEVEL_DBG);

K_SEM_DEFINE(start, 0, 1);

struct k_fifo *framed_telegram_queue;
struct k_pipe *input_pipe;

static struct telegram_framer *telegram_framer;

static struct line_log *line_log;

int framer_task_init(struct k_pipe *input, struct k_fifo *output) {
    input_pipe = input;
    framed_telegram_queue = output;

    line_log = line_log_init();
    if (line_log == NULL) {
        LOG_ERR("Failed to initialize line log");
        return -1;
    }
    telegram_framer = telegram_framer_init();
    if (telegram_framer == NULL) {
        LOG_ERR("Failed to initialize telegram framer");
        return -1;
    }
    k_sem_give(&start);
    return 0;
}

void framer_task(void *user_data) {
	uint8_t c;
    size_t bytes_read;
    k_sem_take(&start, K_FOREVER);
    LOG_INF("Telegram framer task started");
    while(true) {
        k_timeout_t timeout = telegram_framer_is_empty(telegram_framer) ? K_FOREVER : K_MSEC(READ_TIMEOUT_MS);
        int ret = k_pipe_get(input_pipe, &c, 1, &bytes_read, 1, timeout);
        if (ret < 0) {
            if (ret == -EINVAL) {
                LOG_ERR("Invalid argument, should not happen");
                return;
            }
            LOG_DBG("Discarding frame due to timeout");
            telegram_framer_reset(telegram_framer);
            line_log_reset(line_log);
        } else {
            line_log_push(line_log, c);
            struct net_buf *frame = telegram_framer_push(telegram_framer, c);
            if (frame != NULL) {
                net_buf_put(framed_telegram_queue, frame);
            }
	    }
    }
}

#define STACKSIZE 1024
#define PRIORITY 7

K_THREAD_DEFINE(framer_task_thread, STACKSIZE,
                framer_task, NULL, NULL, NULL,
                PRIORITY, 0, 0);
