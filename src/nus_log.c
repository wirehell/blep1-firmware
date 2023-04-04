#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
#include <zephyr/logging/log_backend.h>
#include <zephyr/logging/log_output.h>
#include <zephyr/logging/log_backend_std.h>

#include <sys/param.h>

#include "nus_log.h"

#if 0

#define PIPE_BUFFER_SIZE 4096
K_PIPE_DEFINE(log_pipe, PIPE_BUFFER_SIZE, 4);

#define MAX_BUF_SIZE 256
static uint8_t output_buf[MAX_BUF_SIZE];

int dropped = 0;

int nus_log_output_func(uint8_t *buf, size_t size, void *ctx) {
  /* 
  If the log output function cannot process all of the data, it is its responsibility to mark them
  as dropped or discarded by returning the corresponding number of bytes dropped or discarded to the caller. */
  size_t bytes_written;
  k_pipe_put(&log_pipe, buf, size, &bytes_written, size, K_NO_WAIT);
  dropped += size - bytes_written;

  return size;
}

LOG_OUTPUT_DEFINE(nus_log_output, nus_log_output_func, output_buf, MAX_BUF_SIZE);

void nus_log_process(const struct log_backend *const backend, union log_msg_generic *msg) {
	uint32_t flags = (LOG_OUTPUT_FLAG_LEVEL | LOG_OUTPUT_FLAG_TIMESTAMP | LOG_OUTPUT_FLAG_FORMAT_TIMESTAMP);
  if (log_msg_get_level(&msg->log) <= LOG_LEVEL_INF) {
    log_output_msg_process(&nus_log_output, &msg->log, flags); 
  }
}

static void nus_log_init(struct log_backend const *const backend) {
}

static int nus_log_is_ready(struct log_backend const *const backend) {
  return 0; 
}

static void nus_log_panic(struct log_backend const *const backend) {
  printk("IN panic");
  log_backend_std_panic(&nus_log_output);
}

static void nus_log_dropped(const struct log_backend *const backend, uint32_t cnt) {
  printk("Dropped: %d", cnt);
}

static struct log_backend_api nus_log_backend_api = {
  .process = &nus_log_process,
  .init = &nus_log_init,
  .is_ready = &nus_log_is_ready,
  .panic = &nus_log_panic,
  .dropped = &nus_log_dropped,
};

LOG_BACKEND_DEFINE(nus_log_backend, nus_log_backend_api, true);

static ssize_t read_tx(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf, uint16_t len, uint16_t offset) {
  uint8_t tx_buf[512];
  size_t bytes_read;
  k_pipe_get(&log_pipe, tx_buf, 512, &bytes_read, 1, K_NO_WAIT);
  printk("READ_TX: Read %d bytes\n", bytes_read);
  return bt_gatt_attr_read(conn, attr, buf, len, offset, tx_buf, bytes_read);
}

static void ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value) {
  printk("ccc changed, new value: %d", value);
}

const struct bt_gatt_cpf TX_CPF = {
  .format = 0x19,
};

static struct bt_gatt_attr attrs[] = {
  BT_GATT_PRIMARY_SERVICE(BT_UUID_NUS_SERVICE),
  BT_GATT_CHARACTERISTIC(BT_UUID_NUS_TX,
    BT_GATT_CHRC_READ |BT_GATT_CHRC_NOTIFY,
    BT_GATT_PERM_READ,
    read_tx, NULL, NULL),
  BT_GATT_CCC(ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
  BT_GATT_CPF(&TX_CPF),
  BT_GATT_CHARACTERISTIC(BT_UUID_NUS_RX,
    BT_GATT_CHRC_WRITE | BT_GATT_CHRC_WRITE_WITHOUT_RESP,
    BT_GATT_PERM_WRITE,
    NULL, NULL, NULL), 
};

struct bt_gatt_service nus_log_service = BT_GATT_SERVICE(attrs);

//void bt_conn_foreach(int type, void (*func)(struct bt_conn *conn, void *data), void *data)

struct mtu_info {
  int min_mtu;
  int conn_count;
};

void update_min_mtu(struct bt_conn *conn, void *data) {
  struct mtu_info *mtu_info = data;
  struct bt_conn_info info;

  bt_conn_get_info(conn, &info);
  if (info.state == BT_CONN_STATE_CONNECTED) {
    mtu_info->conn_count++;
    int mtu = bt_gatt_get_mtu(conn);
    mtu_info->min_mtu = MIN(mtu, mtu_info->min_mtu);
  }
}

int get_min_mtu() {
  struct mtu_info mtu_info = {
    .min_mtu = MAX_BUF_SIZE,
    .conn_count = 0,
    };

  bt_conn_foreach(BT_CONN_TYPE_ALL, &update_min_mtu, &mtu_info);

  if (mtu_info.conn_count <= 0) {
    return -1;
  }
  return mtu_info.min_mtu;
}

#define STACKSIZE 1024
#define PRIORITY 8

char SEND_BUF[MAX_BUF_SIZE];

void log_shipping(void *, void *, void *) {
    while (true) {
      int size = get_min_mtu() - 3;
      if (size < 0) {
        // Nothing connected, MTU, normally 23 at least.
        k_sleep(K_MSEC(333));
        continue;
      }
      size_t bytes_read;
      int ret = k_pipe_get(&log_pipe, SEND_BUF, size, &bytes_read, 0, K_MSEC(200));
      if (ret < 0) {
        printk("Log shipping error: %d", ret);
      } else {
        if (bytes_read > 0) {
          bt_gatt_notify(NULL, &nus_log_service.attrs[1], SEND_BUF, bytes_read);
        }
      }
    }
}

K_THREAD_DEFINE(log_shipping_thread, STACKSIZE,
                log_shipping, NULL, NULL, NULL,
                PRIORITY, 0, 0);


#endif