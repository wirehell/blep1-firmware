
#include "modbus.h"
#include "lib/value_store.h"
#include "lib/blep1.h"
#include "udp.h"
#include "tcp.h"

#include <stdint.h>
#include <zephyr/modbus/modbus.h>

LOG_MODULE_REGISTER(modbus_server, LOG_LEVEL_DBG);

K_SEM_DEFINE(response_ready, 0, 1);
 
static int server_iface = -1;
static struct value_store *value_store;
static struct value_store value_store_snapshot;

static uint16_t read_word(struct data_item *data_item, uint16_t offset) {
	uint8_t buf[14]; // TODO max length should be defined somewhere.
    union data_value *value = &data_item->value;
	switch(data_definition_table[data_item->item].format) {
        case DATE_TIME_STRING:
			memcpy(buf, value->date_time, sizeof(value->date_time));
			break;
        case DOUBLE_LONG_UNSIGNED_8_3:
        case DOUBLE_LONG_UNSIGNED_4_3:
			sys_put_be32(value->double_long_unsigned, buf);
			break;
        case LONG_SIGNED_3_1:
        case LONG_UNSIGNED_3_1:
			// Shouldn't be needed, but let's keep it for consistency and to avoid any byte ordering bugs..
			sys_put_be16(value->long_unsigned, buf);
			break;
		default:
			LOG_ERR("BUG: Unhandled item");
			return 0;
	}
	return sys_get_be16(&buf[offset * 2]);
}

static int input_reg_rd(uint16_t addr, uint16_t *reg) {

	if (addr < 0x0800) {
		LOG_WRN("Trying to read non-implemented system registers");
		return -1;
	} 

	uint16_t value_addr = addr - DATA_BASE_ADDRESS;
	uint16_t item = value_addr / 32;
	uint16_t item_offset = value_addr % 32;

	struct value_store_read_result read_result = value_store_read(&value_store_snapshot, item);

	switch (read_result.status) {
		case INVALID:
			LOG_WRN("Read failure; invalid value");
			return -1; 
		case STALE:
			LOG_INF("Read failure; stale value");
			return -1;
		case OK:
			break;
		default:
			LOG_ERR("Read failure: unknown status");
			return -1;
	}

	struct data_item *data_item = &read_result.data.data;
	uint16_t size = data_item_size(data_item);

	if (item_offset > (size-1) /2 ) {
		LOG_ERR("Read failure: invalid offset");
		return -1;
	}

	uint16_t value = read_word(data_item, item_offset);
	LOG_INF("Modbus read input register, 0x%x = %d", addr, value);
	*reg = value;
	return 0;
}

static struct modbus_adu rx_adu;
static struct modbus_adu tx_adu;

static int respond(const int iface, const struct modbus_adu *adu, void *user_data) {
	LOG_INF("Server raw callback from interface %d", iface);
	if (SEND_BUFFER_SIZE < adu->length + MODBUS_MBAP_AND_FC_LENGTH) {
		// TODO set better exception type
		LOG_WRN("Response too large");
		modbus_raw_set_server_failure(&tx_adu);
		k_sem_give(&response_ready);
		return 0;
	}

	tx_adu.trans_id = adu->trans_id;
	tx_adu.proto_id = adu->proto_id;
	tx_adu.length = adu->length;
	tx_adu.unit_id = adu->unit_id;
	tx_adu.fc = adu->fc;
	memcpy(tx_adu.data, adu->data, adu->length);
	LOG_HEXDUMP_INF(tx_adu.data, tx_adu.length, "resp");
	k_sem_give(&response_ready);

	return 0;
}

static struct modbus_user_callbacks muc = {
    .input_reg_rd = input_reg_rd,
};

const static struct modbus_iface_param server_param = {
	.mode = MODBUS_MODE_RAW,
	.server = {
		.user_cb = &muc,
		.unit_id = 1,
	},
	.rawcb.raw_tx_cb = respond,
	.rawcb.user_data = NULL
};

static int send_reply(int socket) {
	uint8_t tx_buf[SEND_BUFFER_SIZE];
	modbus_raw_put_header(&tx_adu, tx_buf);
	// Size is checked earlier
	memcpy(&tx_buf[MODBUS_MBAP_AND_FC_LENGTH], tx_adu.data, tx_adu.length);

	#if CONFIG_BLEP_UDP
	return udp_server_send(tx_buf, MODBUS_MBAP_AND_FC_LENGTH + tx_adu.length);
	#elif CONFIG_BLEP_TCP
	return tcp_server_send(socket, tx_buf, MODBUS_MBAP_AND_FC_LENGTH + tx_adu.length);
	#else
	LOG_ERR("Unknown transport");
	return -1;
	#endif

}

#if CONFIG_BLEP_UDP
static int on_message_received(struct request *request) {
#else
static int on_message_received(struct tcp_request *request) {
#endif
	// Read header
	if (request->len < MODBUS_MBAP_AND_FC_LENGTH) {
		LOG_WRN("Received incomplete header");
		return -1;
	}

	modbus_raw_get_header(&rx_adu, request->recv_buffer);
	LOG_HEXDUMP_DBG(request->recv_buffer, sizeof(MODBUS_MBAP_AND_FC_LENGTH), "h:>");

	if (request->len < MODBUS_MBAP_AND_FC_LENGTH + rx_adu.length) {
		LOG_WRN("Received incomplete ADU");
		return -1;
	}

	memcpy(rx_adu.data, &request->recv_buffer[MODBUS_MBAP_AND_FC_LENGTH], rx_adu.length);

	// Take a snapshot of the value_store
	value_store_copy(value_store, &value_store_snapshot);

	if (modbus_raw_submit_rx(server_iface, &rx_adu)) {
		LOG_ERR("Failed to submit raw ADU");
		return -EIO;
	}

    // We trust our network to not DOS us with garbage
    // TODO: file a request for a dropped message callback
	if (k_sem_take(&response_ready, K_MSEC(500)) != 0) {
		LOG_ERR("Wait time for response expired");
		modbus_raw_set_server_failure(&tx_adu);
	}

	#if CONFIG_BLEP_UDP
	return send_reply(0);
	#else
	return send_reply(request->socket);
	#endif
}

#if CONFIG_BLEP_UDP
struct message_handler handler = {
	.on_message_recived_cb = on_message_received,
};
#elif CONFIG_BLEP_TCP
struct tcp_message_handler handler = {
	.on_message_recived_cb = on_message_received,
};
#endif


int modbus_init(struct value_store *store) {

	value_store = store;

	#if CONFIG_BLEP_UDP
    if (udp_server_init(&handler) < 0) {
        return -1;
    }
	#elif CONFIG_BLEP_TCP
	if (tcp_server_init(&handler) < 0) {
		return -1;
	}
	#else
	LOG_ERR("Unknown transport");
	return -1;
	#endif
    
	char iface_name[] = "RAW_0";
	server_iface = modbus_iface_get_by_name(iface_name);

	if (server_iface < 0) {
		LOG_ERR("Failed to get iface index for %s", iface_name);
		return -ENODEV;
	}

	return modbus_init_server(server_iface, server_param);
}