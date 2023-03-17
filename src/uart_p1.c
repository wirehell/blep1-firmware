#include <zephyr/drivers/uart.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/buf.h>
#include <zephyr/sys/crc.h>
#include <zephyr/kernel.h>
#include <stdlib.h>
#include <sys/types.h>
#include <regex.h>

#include "uart_p1.h"
#include "blep1.h"
#include "line_log.h"
#include "telegram_framer.h"

#define READ_TIMEOUT_MS 200
#define MAX_TELEGRAM_SIZE 8192
#define TELEGRAM_BUF_POOL_SIZE 4
#define BUF_SIZE 256

LOG_MODULE_REGISTER(uart_p1, LOG_LEVEL_DBG);

#if CONFIG_BLEP1_SERIAL

#define UART_DEVICE_NODE DT_NODELABEL(uart1)
//DT_CHOSEN(p1)

K_FIFO_DEFINE(telegram_rx_queue);
K_TIMER_DEFINE(uart_rx_timer, NULL, NULL);

static const struct device *const uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);

static const struct uart_config uart_cfg = {
    .baudrate = 115200,
    .parity = UART_CFG_PARITY_NONE,
    .stop_bits = UART_CFG_STOP_BITS_1,
    .data_bits = UART_CFG_DATA_BITS_8,
    .flow_ctrl = UART_CFG_FLOW_CTRL_NONE
};

static bool initialized;

struct k_pipe *output_pipe;


void receive_cb(const struct device *dev, void *user_data) {
	uint8_t buf[BUF_SIZE];
    int bytes_read;

	if (!uart_irq_update(uart_dev)) {
		return;
	}

	if (!uart_irq_rx_ready(uart_dev)) {
		return;
	}

	/* read until FIFO empty */
    do {
        int bytes_written;
	    bytes_read = uart_fifo_read(uart_dev, &buf, BUF_SIZE);
        int ret = k_pipe_put(output_pipe, buf, bytes_read, &bytes_written, bytes_read, K_NO_WAIT);
        if (ret < 0) {
            LOG_WRN("Buffer overrun, %d bytes", bytes_read);
        }
    } while (bytes_read > 0);
}

int uart_p1_init(struct k_pipe *output) {
    int ret;
    output_pipe = output;

    LOG_INF("Uart init");
    if (!device_is_ready(uart_dev)) {
		return -ENODEV;
	}
    ret = uart_configure(uart_dev, &uart_cfg);
	if (ret < 0) {
        return ret;
	}
	ret = uart_irq_callback_user_data_set(uart_dev, receive_cb, NULL);

	if (ret < 0) {
        return ret;
	}
	uart_irq_rx_enable(uart_dev);

    initialized = 1;
    return 0;
}

// For test TX thread

#define STACKSIZE 1024
#define PRIORITY 7

// Test data
const uint8_t *test_data[] = {
    "/ASD5id123\r\n\r\n",
    "1-0:1.8.0(00006678.394*kWh)\r\n",
    "1-0:2.8.0(00000000.000*kWh)\r\n",
    "1-0:2.9.0(00000000.000)\r\n",
    "1-0:21.7.0(0001.023*kW)\r\n",
    "!10bc\r\n"
};

void test_tx(void *, void *, void *) {
    while (true) {
        k_sleep(K_MSEC(3000));
        if (initialized) {
            LOG_INF("Transmitting");
            for (int i = 0 ; i < sizeof(test_data) / sizeof(*test_data) ; i++) {
                const uint8_t *s = test_data[i];
                for (int j = 0 ; j < strlen(s) ; j++) {
                    uart_poll_out(uart_dev, s[j]);
                }
            }
        }
    }
}

K_THREAD_DEFINE(test_tx_thread, STACKSIZE,
                test_tx, NULL, NULL, NULL,
                PRIORITY, 0, 0);

    
#endif
