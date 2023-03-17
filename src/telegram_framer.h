#ifndef TELEGRAM_FRAMER_HEADER_H
#define TELEGRAM_FRAMER_HEADER_H

#include <zephyr/net/buf.h>

#define MAX_TELEGRAM_SIZE 8192
#define TELEGRAM_BUF_POOL_SIZE 4

struct telegram_framer {
    struct net_buf *buf;
    int pos;
};

struct telegram_framer * telegram_framer_init();

struct net_buf * telegram_framer_push(struct telegram_framer *framer, char c);

void telegram_framer_reset(struct telegram_framer *framer);

void telegram_framer_free(struct telegram_framer *framer);

bool telegram_framer_is_empty(struct telegram_framer *framer); 

#endif /* TELEGRAM_FRAMER_HEADER_H */