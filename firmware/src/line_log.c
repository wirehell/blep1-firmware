#include "line_log.h"
#include "lib/common.h"

#include "zephyr/logging/log.h"

LOG_MODULE_REGISTER(line_log, LOG_LEVEL_INF);

struct line_log * line_log_init() {
    struct line_log *log = common_heap_alloc(sizeof(struct line_log));
    if (log == NULL) {
        LOG_ERR("Failed to allocate line log");
        return NULL;
    }
    log->buf = common_heap_alloc(sizeof(char) * LINE_BUF_SIZE);
    if (log->buf == NULL) {
        LOG_ERR("Failed to allocate line log buffer");
    }
    log->pos = 0;
    return log;
}

void line_log_push(struct line_log *log, char c) {
    // Line logging for debugging
    int pos = log->pos;
    if (pos < LINE_BUF_SIZE) {
        log->buf[pos] = c;
        if (pos >= 2 && log->buf[pos - 1] == '\r' && c == '\n') {
            log->buf[pos-1] = '\0';
            LOG_INF("Recv: %s", log->buf);
            log->pos = 0;
        } else {
            log->pos = pos + 1;
        }
    } else {
        log->buf[LINE_BUF_SIZE - 1] = '\0';
        LOG_INF("Received too long line: %s..", log->buf);
        log->pos = 0;
    }
}

void line_log_reset(struct line_log *log) {
    log->pos = 0;
}

void line_log_free(struct line_log *log) {
    common_heap_free(log->buf);
    common_heap_free(log);
}