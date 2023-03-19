#include "telegram_framer.h"
#include "common.h"

#include <zephyr/sys/crc.h>
#include <zephyr/kernel.h>
#include <zephyr/net/buf.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(telegram_framer, LOG_LEVEL_DBG);

NET_BUF_POOL_DEFINE(telegram_buf_pool, TELEGRAM_BUF_POOL_SIZE, MAX_TELEGRAM_SIZE, 0, NULL);

struct telegram_framer * telegram_framer_init() {
    struct telegram_framer *framer = common_heap_alloc(sizeof(struct telegram_framer));
    if (framer == NULL) {
        LOG_ERR("Could not allocate telegram_frame");
        return NULL;
    }
    struct net_buf *buf = net_buf_alloc(&telegram_buf_pool, K_NO_WAIT);
    if (buf == NULL) {
        LOG_ERR("Could not allocate telegram_frame buffer");
        common_heap_free(framer);
        return NULL;
    }
    framer->buf = buf;
    framer->pos = 0;
    return framer;
}

bool telegram_framer_check_complete(struct telegram_framer *framer) {
    int pos = framer->pos;
    uint8_t *buf = framer->buf->data;
    // Check for \r\n!XXYY\r\n on end
    if (pos < 6 || buf[pos-1] != '\n' || buf[pos - 2] != '\r' || buf[pos - 7] != '!' || buf[pos - 8] != '\n' || buf[pos - 9] != '\r' ) {
        return false;
    }
    return true;
}

void telegram_framer_remove_footer(struct telegram_framer *framer) {
    net_buf_remove_mem(framer->buf, 7); // Remove footer
    net_buf_add_u8(framer->buf, '\0');
}

struct net_buf * telegram_framer_swap_buf(struct telegram_framer *framer) {
    // Allocate next, if not available, reset and reuse this buffer.
    struct net_buf *next_buf = net_buf_alloc(&telegram_buf_pool, K_NO_WAIT);
    if (next_buf == NULL) {
        LOG_WRN("Could not allocate next telegram_frame buffer, discarding and reusing current");
        telegram_framer_reset(framer);
        return NULL;
    }
    struct net_buf *current = framer->buf;
    framer->buf = next_buf;
    framer->pos = 0;
    return current;
}

// Assumes telegram_framer_check_complete() is true
bool telegram_framer_verify_checksum(struct telegram_framer *framer) {
    int pos = framer->pos;
    uint8_t *data = framer->buf->data;
    if (pos < 7 || data[pos - 7] != '!') {
        LOG_ERR("Malformed telegram, should not happen");
        return false;
    }
    char *checksum_start = &data[pos-6];
    char *checksum_end = &data[pos-3];

    unsigned long parsed = strtoul(checksum_start, &checksum_end, 16);
    if (parsed == ULONG_MAX) {
        LOG_HEXDUMP_INF(&data[pos - 7], 7, "Checksum parse error");
        return false;
    }
    uint16_t checksum_parsed = (uint16_t) parsed;
    LOG_INF("Parsed checksum: %x", checksum_parsed);
    uint16_t checksum_computed = crc16_reflect(0xa001, 0, framer->buf->data, pos-6);
    if (checksum_computed != checksum_parsed) {
        LOG_INF("CRC error. computed: %x != parsed: %x", checksum_computed, checksum_parsed);
        return false;
    }
    return true;
}


struct net_buf * telegram_framer_push(struct telegram_framer *framer, char c) {
    int pos = framer->pos;

    if (pos >= MAX_TELEGRAM_SIZE) {
        LOG_WRN("Overflow, discarding buffer");
        telegram_framer_reset(framer);
    }

    if (pos == 0 && c != '/') {
        // Discard intial non-start characters
        // Todo add metric
        return NULL;
    }

    if (net_buf_add_u8(framer->buf, c) == NULL) {
        LOG_ERR("Could not add element, resetting framer");
        telegram_framer_reset(framer);
    }

    framer->pos++;

    if (telegram_framer_check_complete(framer)) {
        LOG_INF("Received telegram frame, message length: %u", framer->buf->len);

        if (!telegram_framer_verify_checksum(framer)) {
            LOG_WRN("Checksum failure");
            telegram_framer_reset(framer);
            return NULL;
        }

        telegram_framer_remove_footer(framer);
        return telegram_framer_swap_buf(framer);
    }

    return NULL;

}

void telegram_framer_reset(struct telegram_framer *framer) {
    LOG_DBG("Resetting frame, discarding %d bytes", framer->pos);
    net_buf_reset(framer->buf);
    framer->pos = 0;
}

void telegram_framer_free(struct telegram_framer *framer) {
    net_buf_unref(framer->buf);
    common_heap_free(framer);
}

bool telegram_framer_is_empty(struct telegram_framer *framer) {
    return 0 == framer->pos;
}