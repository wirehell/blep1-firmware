#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "lib/blep1.h"
#include "lib/parser.h"
#include "lib/telegram.h"

#include "uart_p1.h"
#include "framer_task.h"
#include "parser_task.h"
#include "handler_task.h"

#include "lib/value_store.h"
#include "modbus_udp.h"
//#include "ble.h"

K_PIPE_DEFINE(rx_pipe, 4096, 4);
K_PIPE_DEFINE(tx_pipe, 4096, 4);
K_FIFO_DEFINE(telegram_frame_fifo);
K_MSGQ_DEFINE(telegram_queue, sizeof(telegram_message), 1, 4); 

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

//static struct ble_service ble_service;

struct value_store value_store;

void update_data_store(struct data_item *data_item) {
	value_store_update(&value_store, data_item);
//	ble_service_update_item(&ble_service, data_item);
}

void main(void) {
	int err;
	LOG_INF("Starting..");

#if CONFIG_BLEP1_SERIAL
	err = uart_p1_init(&rx_pipe, &tx_pipe);
	if (err < 0) {
		LOG_ERR("Could not init uart (err %d)", err);
		return;
	}
#endif

	err = framer_task_init(&rx_pipe, &telegram_frame_fifo);
	if (err < 0) {
		LOG_ERR("Could not init parser task (err %d)", err);
		return;
	}

	err = parser_task_init(&telegram_frame_fifo, &telegram_queue);
	if (err < 0) {
		LOG_ERR("Could not init parser task (err %d)", err);
		return;
	}

/*
	err = bt_enable(NULL);
	if (err < 0) {
		LOG_ERR("Bluetooth init failed (err %d)", err);
		return;
	}

	err = ble_service_init(&ble_service);
	if (err < 0) {
		LOG_ERR("BLE service init failed (err %d)", err);
		return;
	}
*/

	err = handler_task_init(&telegram_queue, &update_data_store);
	if (err < 0) {
		LOG_ERR("Could not init handler task (err %d)", err);
		return;
	}

	err = modbus_udp_init(&value_store);
	if (err < 0) {
		LOG_ERR("Could not initalize UDP Modbus server");
		return;
	}
//	bt_ready();

	LOG_INF("Up and running..");

	while (true) {
		k_sleep(K_SECONDS(1));
	}
}
