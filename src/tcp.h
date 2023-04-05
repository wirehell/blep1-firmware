#ifndef TCP_HEADER_H
#define TCP_HEADER_H

#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <sys/types.h>

#define MODBUS_PORT 502
#define RECV_BUFFER_SIZE 32
#define SEND_BUFFER_SIZE 32
#define MAX_CLIENT_QUEUE

struct tcp_server {
  int sock;
  struct tcp_message_handler *handler;
};

struct tcp_request {
  int socket;
  uint8_t len;
  uint8_t recv_buffer[RECV_BUFFER_SIZE];
};

typedef int (*on_tcp_message_recived_t)(struct tcp_request *req);

struct tcp_message_handler {
  on_tcp_message_recived_t on_message_recived_cb;
};

int tcp_server_init(struct tcp_message_handler *handler);
int tcp_server_send(int socket, uint8_t *buf, int len);

#endif /* TCP_HEADER_H */