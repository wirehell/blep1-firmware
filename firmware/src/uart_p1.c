#include <zephyr/drivers/uart.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/buf.h>
#include <zephyr/kernel.h>
#include <sys/types.h>

#include "uart_p1.h"

#if CONFIG_OPENP1_SERIAL

#define READ_TIMEOUT_MS 200
#define MAX_TELEGRAM_SIZE 8192
#define TELEGRAM_BUF_POOL_SIZE 4
#define BUF_SIZE 256

LOG_MODULE_REGISTER(uart_p1, LOG_LEVEL_DBG);

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

static struct k_pipe *output_pipe;
static struct k_pipe *tx_pipe;

static const int read_buf_size = BUF_SIZE;

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
	    bytes_read = uart_fifo_read(uart_dev, buf, read_buf_size);
        int ret = k_pipe_put(output_pipe, buf, bytes_read, &bytes_written, bytes_read, K_NO_WAIT);
        if (ret < 0) {
            LOG_WRN("Buffer overrun, %d bytes", bytes_read - bytes_written);
        }
    } while (bytes_read > 0);
}

int uart_p1_init(struct k_pipe *output, struct k_pipe *tx) {
    int ret;
    output_pipe = output;
    tx_pipe = tx;

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

    LOG_INF("Uart initialized");
    initialized = 1;
    return 0;
}

#if CONFIG_OPENP1_SIM_UART_OUT

#define STACKSIZE 1024
#define PRIORITY 7

#define TX_BUF_SIZE 256

uint8_t TX_BUF[TX_BUF_SIZE];

void test_tx(void *, void *, void *) {
    while (!initialized) {
        k_sleep(K_MSEC(100));
    }
    while (true) {
        size_t bytes_read;
        int ret = k_pipe_get(tx_pipe, TX_BUF, TX_BUF_SIZE, &bytes_read, 1, K_MSEC(100));
        if (ret == -EAGAIN) {
            continue;
        } else if (ret < 0) {
            LOG_ERR("Failed to read data from tx_pipe %d. Aborting transmissions.", ret);
            return;
        }
        LOG_INF("Transmitting test data: %d", bytes_read);
        for (int i = 0 ; i < bytes_read ; i++) {
            uart_poll_out(uart_dev, TX_BUF[i]);
        }
    }
}

K_THREAD_DEFINE(test_tx_thread, STACKSIZE,
                test_tx, NULL, NULL, NULL,
                PRIORITY, K_ESSENTIAL, 0);

#endif
    
#endif
