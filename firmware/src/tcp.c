#include "tcp.h"

#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/modbus/modbus.h>

#include <sys/types.h>

#if CONFIG_OPENP1_TCP

LOG_MODULE_REGISTER(tcp, LOG_LEVEL_DBG);

#define STACK_SIZE 2048
#define RECEIVE_THREAD_PRIORITY 8
#define MAX_CONNECTIONS 8

K_THREAD_STACK_ARRAY_DEFINE(tcp_handler_stack, MAX_CONNECTIONS, STACK_SIZE);
K_SEM_DEFINE(request_possible, 1, 1);

static struct tcp_server server;

static struct tcp_request request[MAX_CONNECTIONS];

static struct k_thread tcp_handler_thread[MAX_CONNECTIONS];
static int tcp_sockets_in_use[MAX_CONNECTIONS];
uint8_t recieve_buf[MAX_CONNECTIONS][RECV_BUFFER_SIZE];

static int find_free_slot() {
  for (int i = 0 ; i < MAX_CONNECTIONS ; i++) {
    if (tcp_sockets_in_use[i] < 0) {
      return i;
    }
  }
  return -1;
}

static void handle_tcp_connection(void *ptr1, void *ptr2, void *ptr3) {
  int slot = POINTER_TO_INT(ptr1);
  int ret = 0;
  int client = tcp_sockets_in_use[slot];
  struct modbus_adu adu;
  uint8_t *buf = request[slot].recv_buffer;

  struct timeval receive_timeout = {
    .tv_sec = 120,
    .tv_usec = 0,
  };
  ret = setsockopt(client, SOL_SOCKET, SO_SNDTIMEO, &receive_timeout, sizeof(receive_timeout));

  struct timeval send_timeout = {
    .tv_sec = 10,
    .tv_usec = 0,
  };
  ret = setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, &send_timeout, sizeof(send_timeout));

  LOG_INF("Waiting for TCP packets on port on socket: %d", client);

  do {
    ret = recv(client, buf, MODBUS_MBAP_AND_FC_LENGTH, MSG_WAITALL);
    if (ret != MODBUS_MBAP_AND_FC_LENGTH) {
      LOG_WRN("Error receiving modbus header, closing socket");
      break;
    }

    LOG_HEXDUMP_DBG(buf, MODBUS_MBAP_AND_FC_LENGTH , "h:>");
	  modbus_raw_get_header(&adu, buf);

    ret = recv(client, buf + MODBUS_MBAP_AND_FC_LENGTH, adu.length, MSG_WAITALL);
    if (ret != adu.length) {
      LOG_WRN("Error receiving modbus data, closing socket");
      break;
    }
    request[slot].socket = client;
    request[slot].len = MODBUS_MBAP_AND_FC_LENGTH + adu.length;

    // Handle request
    if (server.handler->on_message_recived_cb(&request[slot]) < 0) {
      LOG_ERR("Failed to handle request");
      break;
    }

  } while (true);

  close(client);
  tcp_sockets_in_use[slot] = -1;
}

static int process_connect() {

	struct sockaddr_in6 client_addr;
	socklen_t client_addr_len = sizeof(client_addr);

	LOG_INF("Waiting for TCP connection on port %d...", MODBUS_PORT);

	int client = accept(server.sock, (struct sockaddr *)&client_addr, &client_addr_len);
	if (client < 0) {
		LOG_ERR("Accept error");
		return 0;
	}

	int slot = find_free_slot();
	if (slot < 0) {
		LOG_ERR("Cannot accept more connections");
		close(client);
		return 0;
	}

  tcp_sockets_in_use[slot] = client;

  char buf[128];
  if (net_addr_ntop(AF_INET6, &client_addr.sin6_addr, buf, 128) == NULL) {
    LOG_ERR("Couldn't put address in string");
    tcp_sockets_in_use[slot] = -1;
    return 0;
  } else {
  	LOG_INF("Accepted connection from %s", buf);
  }


  k_thread_create(
    &tcp_handler_thread[slot],
    tcp_handler_stack[slot],
    K_THREAD_STACK_SIZEOF(tcp_handler_stack[slot]),
    (k_thread_entry_t)handle_tcp_connection,
    INT_TO_POINTER(slot), NULL, NULL,
    RECEIVE_THREAD_PRIORITY,
    0,
    K_NO_WAIT);

	char name[16];
  snprintk(name, sizeof(name), "tcp[%d]", slot);
  k_thread_name_set(&tcp_handler_thread[slot], name);

  return 0;
}

static void tcp_connection_handler_task() {
  int ret = 0;
  do {
    ret = process_connect();
  } while (ret == 0);
  LOG_ERR("Conneciton handler task finished");
}

int tcp_server_send(int socket, uint8_t *buf, int len) {
  LOG_DBG("Sending reply");
  int ret = send(socket, buf, len, 0);
  if (ret < 0) {
    LOG_ERR("Failed to send %d", errno);
    ret = -errno;
  }
  LOG_INF("Sent reply: %d bytes", len);
  return 0;
}

K_THREAD_DEFINE(tcp_thread_id, STACK_SIZE,
    tcp_connection_handler_task, NULL, NULL, NULL,
    RECEIVE_THREAD_PRIORITY, 0, -1);

int tcp_server_init(struct tcp_message_handler *handler) {

  int ret;
  server.handler = handler;
  struct sockaddr_in6 addr;

  for (int i = 0 ; i < MAX_CONNECTIONS ; i++) {
    tcp_sockets_in_use[i] = -1;
  }

  (void)memset(&addr, 0, sizeof(addr));
  addr.sin6_family = AF_INET6;
  addr.sin6_port = htons(MODBUS_PORT);

  server.sock = socket(addr.sin6_family, SOCK_STREAM, IPPROTO_TCP);
  if (server.sock < 0) {
    LOG_ERR("Failed to create socket: %d", errno);
    return -errno;
  }

  ret = bind(server.sock, (struct sockaddr *) &addr, sizeof(addr));
  if (ret < 0) {
    LOG_ERR("Failed to bind socket: %d", errno);
    ret = -errno;
  }

  ret = listen(server.sock, 1);
  if (ret < 0) {
    LOG_ERR("Failed to listen on socket: %d", errno);
    ret = -errno;
  }

  k_thread_name_set(tcp_thread_id, "tcp connection handler");
  k_thread_start(tcp_thread_id);

  LOG_INF("Server initialized");
  return 0;
}

#endif