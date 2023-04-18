#include "tcp_log.h"

#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_backend.h>
#include <zephyr/logging/log_output.h>
#include <zephyr/logging/log_backend_std.h>


#if CONFIG_OPENP1_LOG_TCP

#define PIPE_BUFFER_SIZE 2048
K_PIPE_DEFINE(log_pipe, PIPE_BUFFER_SIZE, 4);

#define MAX_BUF_SIZE 256
static uint8_t output_buf[MAX_BUF_SIZE];

int dropped = 0;

static int tcp_log_output_func(uint8_t *buf, size_t size, void *ctx) {
  /* 
  If the log output function cannot process all of the data, it is its responsibility to mark them
  as dropped or discarded by returning the corresponding number of bytes dropped or discarded to the caller. */
  size_t bytes_written;
  k_pipe_put(&log_pipe, buf, size, &bytes_written, size, K_NO_WAIT);
  dropped += size - bytes_written;

  return size;
}

LOG_OUTPUT_DEFINE(tcp_log_output, tcp_log_output_func, output_buf, MAX_BUF_SIZE);

static void tcp_log_process(const struct log_backend *const backend, union log_msg_generic *msg) {
	uint32_t flags = (LOG_OUTPUT_FLAG_LEVEL | LOG_OUTPUT_FLAG_TIMESTAMP | LOG_OUTPUT_FLAG_FORMAT_TIMESTAMP);
  if (log_msg_get_level(&msg->log) <= LOG_LEVEL_INF) {
    log_output_msg_process(&tcp_log_output, &msg->log, flags); 
  }
}

static void tcp_log_init(struct log_backend const *const backend) {
}

static int tcp_log_is_ready(struct log_backend const *const backend) {
  return 0; 
}

static void tcp_log_panic(struct log_backend const *const backend) {
  printk("IN panic");
  log_backend_std_panic(&tcp_log_output);
}

static void tcp_log_dropped(const struct log_backend *const backend, uint32_t cnt) {
  printk("Dropped: %d", cnt);
}

static struct log_backend_api tcp_log_backend_api = {
  .process = &tcp_log_process,
  .init = &tcp_log_init,
  .is_ready = &tcp_log_is_ready,
  .panic = &tcp_log_panic,
  .dropped = &tcp_log_dropped,
};

LOG_BACKEND_DEFINE(log_tcp_backend, tcp_log_backend_api, true);

#define STACKSIZE 2048
#define PRIORITY 8


#define LOG_MTU 1024
char SEND_BUF[LOG_MTU];

int log_socket = -1;

static int log_shipping(int client) {

  while (true) {
    int size = LOG_MTU;
    size_t bytes_read;
    int ret = k_pipe_get(&log_pipe, SEND_BUF, size, &bytes_read, 0, K_MSEC(200));
    if (ret < 0) {
      printk("Log shipping error: %d\n", ret);
      continue;
    }
    if (bytes_read > 0) {
      ret = send(client, SEND_BUF, bytes_read, 0);
      if (ret < 0) {
        printk("closing client: %d", ret);
        close(client);
        return 0;
      } 
    }
  }
}

static void tcp_connection_handler(void *ptr1, void *ptr2, void *ptr3) {
	struct sockaddr_in6 client_addr;
	socklen_t client_addr_len = sizeof(client_addr);

  while (true) {

    printk("Waiting for TCP connection on port %d...", TCP_LOG_PORT);

    int client = accept(log_socket, (struct sockaddr *)&client_addr, &client_addr_len);
    if (client < 0) {
	    printk("Accept error");
      continue;
    }
    char buf[256];
    if (net_addr_ntop(AF_INET6, &client_addr.sin6_addr, buf, 256) == NULL) {
      printk("Couldn't put address in string");
      close(client);
      continue;
    } else {
      printk("Accepted connection from %s", buf);
    }

    struct timeval timeout;
    timeout.tv_sec = 15;
    timeout.tv_usec = 0;

    int ret = setsockopt(log_socket, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    if (ret < 0) {
      printk("Couldn't set socket timeout");
      close(client);
      continue;
    }

    log_shipping(client);

    k_sleep(K_MSEC(500));

  } 

}

K_THREAD_DEFINE(log_shipping_thread, STACKSIZE,
                tcp_connection_handler, NULL, NULL, NULL,
                PRIORITY, K_ESSENTIAL, -1);


struct sockaddr_in6 addr;

int tcp_log_server_start() {
  int ret;

  (void)memset(&addr, 0, sizeof(addr));
  addr.sin6_family = AF_INET6;
  addr.sin6_port = htons(TCP_LOG_PORT);

  log_socket = socket(addr.sin6_family, SOCK_STREAM, IPPROTO_TCP);
  if (log_socket < 0) {
    printk("Failed to create socket: %d", errno);
    return -errno;
  }

  ret = bind(log_socket, (struct sockaddr *) &addr, sizeof(addr));
  if (ret < 0) {
    printk("Failed to bind socket: %d", errno);
    ret = -errno;
  }

  ret = listen(log_socket, 1);
  if (ret < 0) {
    printk("Failed to listen on socket: %d", errno);
    ret = -errno;
  }

  struct timeval timeout;
  timeout.tv_sec = 15;
  timeout.tv_usec = 0;

  ret = setsockopt(log_socket, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

  k_thread_name_set(log_shipping_thread, "TCP log thread");
  k_thread_start(log_shipping_thread);

  printk("Log server initialized");
  return 0;
}

#endif