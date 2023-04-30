#include "udp.h"

#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>

#if CONFIG_OPENP1_UDP

LOG_MODULE_REGISTER(udp, LOG_LEVEL_DBG);

#define STACK_SIZE 2048
#define RECEIVE_THREAD_PRIORITY 8

static struct request request;

static struct udp_server server;

static void receive_udp_task()
{
  int ret = 0;
  int received;

  LOG_INF("Waiting for UDP packets on port %d...", MODBUS_PORT);

  do {
    request.client_addr_len = sizeof(request.client_addr);
    received = recvfrom(server.sock, request.recv_buffer, RECV_BUFFER_SIZE, MSG_TRUNC,
			&request.client_addr, &request.client_addr_len);

    if (received < 0) {
	    /* Socket error */
	    LOG_ERR("UDP: Connection error %d", errno);
	    ret = -errno;
	    break;
    } else if (received > RECV_BUFFER_SIZE) {
      LOG_WRN("UDP: Truncating packet, len: %d", received);
      continue;
    } 
    
    LOG_INF("UDP: Received %d bytes", received);
    request.len = received;

    // Handle request
    if (server.handler->on_message_recived_cb(&request) < 0) {
      continue;
    }


  } while (true);

}

int udp_server_send(uint8_t *buf, int len) {
  LOG_DBG("Sending reply");
  int ret = sendto(server.sock, buf, len, 0, &request.client_addr, request.client_addr_len);

  if (ret < 0) {
    LOG_ERR("UDP: Failed to send %d", errno);
    ret = -errno;
  }
  LOG_INF("Sent reply: %d bytes", len);
  return 0;
}

K_THREAD_DEFINE(udp_thread_id, STACK_SIZE,
    receive_udp_task, NULL, NULL, NULL,
    RECEIVE_THREAD_PRIORITY, K_ESSENTIAL, -1);

int udp_server_init(struct message_handler *handler) {

  server.handler = handler;
  struct sockaddr_in6 addr;

  (void)memset(&addr, 0, sizeof(addr));
  addr.sin6_family = AF_INET6;
  addr.sin6_port = htons(MODBUS_PORT);

  server.sock = socket(addr.sin6_family, SOCK_DGRAM, IPPROTO_UDP);
  if (server.sock < 0) {
    LOG_ERR("Failed to create UDP socket: %d", errno);
    return -errno;
  }

  int ret = bind(server.sock, (struct sockaddr *) &addr, sizeof(addr));
  if (ret < 0) {
    LOG_ERR("Failed to bind UDP socket: %d", errno);
    ret = -errno;
  }

  k_thread_name_set(udp_thread_id, "udp");
  k_thread_start(udp_thread_id);

  LOG_INF("UDP server initialized");
  return ret;
}

#endif