#ifndef UDP_HEADER_H
#define UDP_HEADER_H

#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <sys/types.h>

#define MODBUS_PORT 502
#define RECV_BUFFER_SIZE 32
#define SEND_BUFFER_SIZE 32
#define MAX_OUTSTANDING_REQUESTS 4

struct udp_server {
  int sock;
  struct message_handler *handler;
};

struct request {
  struct sockaddr client_addr;
  socklen_t client_addr_len;
  uint8_t len;
  uint8_t recv_buffer[RECV_BUFFER_SIZE];
};

typedef int (*on_message_recived_t)(struct request *req);

struct message_handler {
  on_message_recived_t on_message_recived_cb;
};

int udp_server_init(struct message_handler *handler);
int udp_server_send(uint8_t *buf, int len);

#endif /* UDP_HEADER_H */