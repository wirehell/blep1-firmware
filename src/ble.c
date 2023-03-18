#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include "zephyr/bluetooth/services/bas.h"

#include "ble.h"
#include "nus_log.h"

#include "lib/blep1.h"
#include "lib/common.h"

LOG_MODULE_REGISTER(ble, LOG_LEVEL_DBG);

#define BT_UUID_CUSTOM_SERVICE_VAL BT_UUID_128_ENCODE(0xb3b43e2e, 0x1e86, 0x46f5, 0xa156, 0x4ffd0c7c13ac)

static struct bt_uuid_128 blep1_uuid = BT_UUID_INIT_128(BT_UUID_CUSTOM-SERVICE_VAL);

static struct bt_uuid_16 CHRC_UUID = BT_UUID_INIT_16(BT_UUID_GATT_CHRC_VAL);
static struct bt_uuid_16 CUD_UUID = BT_UUID_INIT_16(BT_UUID_GATT_CUD_VAL);
static struct bt_uuid_16 CPF_UUID = BT_UUID_INIT_16(BT_UUID_GATT_CPF_VAL);
static struct bt_uuid_16 CCC_UUID = BT_UUID_INIT_16(BT_UUID_GATT_CCC_VAL);

static ssize_t read_value(struct bt_conn *conn, const struct bt_gatt_attr *attr,
      void *buf, uint16_t len, uint16_t offset)
{
  struct user_data *user_data = attr->user_data;

  struct data_item data_item = {user_data->item, user_data->value };
  // change interface
  uint16_t size = data_item_size(&data_item);

//  LOG_DBG("Reading item: %d %d", user_data->item, user_data->value);

  return bt_gatt_attr_read(conn, attr, buf, len, offset, &user_data->value, size);
}

#define CPF_FORMAT_UINT8 0x04
#define CPF_FORMAT_UINT32 0x08

#define MAX_ATTRIBUTES 512

const struct bt_uuid_128 METER_ACTIVE_ENERGY_IN_UUID = BT_UUID_INIT_128(
    BT_UUID_128_ENCODE(0xda415e1d, 0xa955, 0x45fc, 0xa9ef, 0xc2c02f95af5c));
const struct bt_uuid_128 METER_ACTIVE_ENERGY_OUT_UUID = BT_UUID_INIT_128(
    BT_UUID_128_ENCODE(0xe93595bf, 0xa138, 0x4827, 0xbae4, 0x8b556bbd8a6d));
const struct bt_uuid_128 METER_REACTIVE_ENERGY_IN_UUID = BT_UUID_INIT_128(
	BT_UUID_128_ENCODE(0x0594194e, 0xa360, 0x451d, 0xbb8e, 0xbbbfe1fd9d27));
const struct bt_uuid_128 METER_REACTIVE_ENERGY_OUT_UUID = BT_UUID_INIT_128(
	BT_UUID_128_ENCODE(0x86a6f8ec, 0x9529, 0x4ef9, 0x8327, 0x5f32eb98e5b7));


const struct bt_uuid_128 ACTIVE_ENERGY_IN_UUID = BT_UUID_INIT_128(
    BT_UUID_128_ENCODE(0xe587eea0, 0xc607, 0x41f9, 0x9a6e, 0x8a748dff8f53));
const struct bt_uuid_128 ACTIVE_ENERGY_OUT_UUID = BT_UUID_INIT_128(
    BT_UUID_128_ENCODE(0x48740983, 0xed21, 0x49fa, 0xa95d, 0xe263d989a031));
const struct bt_uuid_128 REACTIVE_ENERGY_IN_UUID = BT_UUID_INIT_128(
    BT_UUID_128_ENCODE(0x58dcf32d, 0x4a41, 0x4b09, 0xb821, 0x333005971c6b));
const struct bt_uuid_128 REACTIVE_ENERGY_OUT_UUID = BT_UUID_INIT_128(
    BT_UUID_128_ENCODE(0xc6acfc29, 0xa4a0, 0x4241, 0x98aa, 0xec52b37ea1c5));

/*
92852415-0682-4246-93fb-7d0b27d87e13
450144e3-40a5-41ba-94d8-44963d21cea7
0a7dae17-a85f-49a4-9340-d31021e9ec50

f79c914c-4d37-4913-ab85-cda5c87aeb6e
e7fc354e-c420-4c07-99c7-f946215672e9
6c038bf7-fec5-4f1f-b508-f6c902469597

4c29b9f1-39e3-4d3b-a8cf-bbf2174ac6ec
2f9324e3-28f2-4a20-9884-784d6168d69d
6fa7f29f-a6bc-462f-9640-65b27e5a9dda

13b2f440-0fe0-4770-b7e4-956ed0637531
769b7853-e490-48e8-895c-1411746cefcc
*/

#define CPF_UNIT_ENERGY_KWH 0x27AB
#define CPF_UNIT_POWER_W 0x2726

#define CPF_FORMAT_UINT8 0x04
#define CPF_FORMAT_UINT32 0x08

const struct bt_gatt_cpf CPF_UINT32_KWH_FROM_8_3_KWH = {
  .format = CPF_FORMAT_UINT32,
  .exponent = -3,
  .unit = CPF_UNIT_ENERGY_KWH,
};

const struct bt_gatt_cpf CPF_UINT32_W_FROM_4_3_KW = {
  .format = CPF_FORMAT_UINT32,
  .exponent = 0,
  .unit = CPF_UNIT_POWER_W,
};

const struct gatt_ch gatt_ch_table[] = {
    { METER_ACTIVE_ENERGY_IN,   &METER_ACTIVE_ENERGY_IN_UUID,   &CPF_UINT32_KWH_FROM_8_3_KWH, "Meter - Active energy in"},
    { METER_ACTIVE_ENERGY_OUT,  &METER_ACTIVE_ENERGY_OUT_UUID,  &CPF_UINT32_KWH_FROM_8_3_KWH, "Meter - Active energy out"},
    { ACTIVE_ENERGY_IN,         &ACTIVE_ENERGY_IN_UUID,         &CPF_UINT32_W_FROM_4_3_KW,    "Active energy in"},
    { ACTIVE_ENERGY_OUT,        &ACTIVE_ENERGY_OUT_UUID,        &CPF_UINT32_W_FROM_4_3_KW,    "Active energy out"},
};

int ble_service_add_attributes(struct ble_service *ble_service, struct bt_gatt_attr *attributes, int count) {
	struct bt_gatt_attr *service_attributes = ble_service->gatt_service.attrs;
	size_t *index = &(ble_service->gatt_service.attr_count);

	for (int i = 0 ; i < count ; i++) {
		if (*index >= MAX_ATTRIBUTES) {
			LOG_ERR("Max attributes reached: %d", *index);
			return -1;
		}
		// Copy..
		service_attributes[*index] = attributes[i];
		*index += 1;
	}
	return 0;
}

int ble_service_update_item(struct ble_service *service, struct data_item *data_item) {
	int ret;
	struct user_data *data = &service->user_data[data_item->item];
	data->value = data_item->value;
	data->last_updated = 1;
	// TODO possible race condition if value is updated before notification happens. Once notify returns it should no longer referene value.
	ret = bt_gatt_notify(NULL, data->attr, &data->value, data_item_size(data_item));
	if (ret < 0) {
		// This is normal when not connected..
		// LOG_DBG("Failed gatt_notify of item: %d, err: %d", data_item->item, ret);
		return -1;
	}
	return 0;
}

const struct bt_gatt_attr primary_service = BT_GATT_PRIMARY_SERVICE(&blep1_uuid);

int ble_service_add_primary_service(struct ble_service *service) {

	struct bt_gatt_attr attributes[] = { 
		primary_service
	};
	int ret = ble_service_add_attributes(service, attributes, ARRAY_SIZE(attributes));
	if (ret < 0) {
		LOG_ERR("Could not add primary service attribute(s)");
		return -1;
	}
	return 0;
}

int ble_service_add_item_attributes(struct ble_service * service) {
	for (int item = 0; item < _ITEM_COUNT; item++) {
		const struct gatt_ch *ch = &gatt_ch_table[item];
		struct bt_gatt_attr attributes[] = {
			BT_GATT_ATTRIBUTE(&CHRC_UUID.uuid, BT_GATT_PERM_READ, bt_gatt_attr_read_chrc, NULL, &service->chrc[item]),
			BT_GATT_ATTRIBUTE(&ch->uuid->uuid, BT_GATT_PERM_READ, read_value, NULL, &service->user_data[item]),
			BT_GATT_ATTRIBUTE(&CUD_UUID.uuid, BT_GATT_PERM_READ, bt_gatt_attr_read_cud, NULL, ch->description),
			BT_GATT_ATTRIBUTE(&CPF_UUID.uuid, BT_GATT_PERM_READ, bt_gatt_attr_read_cpf, NULL, ch->cpf), 
			BT_GATT_ATTRIBUTE(&CCC_UUID.uuid, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE, 
				bt_gatt_attr_read_ccc, bt_gatt_attr_write_ccc, &service->ccc[item])
		};
		int ret = ble_service_add_attributes(service, attributes, ARRAY_SIZE(attributes));
		if (ret < 0) {
			LOG_ERR("Could not add item attribute: %d", item);
			return -1;
		}
		// Add a self reference for notficationas
		// Assumes the value attribute is third from bottom in list above
		struct bt_gatt_attr *value_attr = &service->gatt_service.attrs[service->gatt_service.attr_count - 4];
		service->user_data[item] = (struct user_data) {
			.item = ch->item,
			.last_updated = 0,
			.value = 0,
			.attr = value_attr,
		};
	}
	return 0;
}

void ble_service_init_chrc(struct ble_service *service) {
	for(int i = 0 ; i < ARRAY_SIZE(service->chrc); i++) {
		service->chrc[i] = (struct bt_gatt_chrc) { 
			.uuid = &(gatt_ch_table[i].uuid->uuid), 
			.value_handle = 0U, 
			.properties = BT_GATT_CHRC_READ  | BT_GATT_CHRC_NOTIFY,
		};
	}
}

void ble_service_init_ccc(struct ble_service *service) {
	for(int i = 0 ; i < ARRAY_SIZE(service->ccc); i++) {
		service->ccc[i] = (struct _bt_gatt_ccc) { 
			.cfg = {},
			.cfg_changed = NULL,
			.cfg_match = NULL,
			.cfg_write = NULL,
		};
	}
}

int ble_service_init(struct ble_service *service) {
	int err;
	// Allocate storage for max attributes, but uses less, saves some implementation time
	service->gatt_service.attrs = common_heap_alloc(sizeof(struct bt_gatt_attr) * MAX_ATTRIBUTES); 
	if (service->gatt_service.attrs == NULL) {
		LOG_ERR("Failed to allocate BLE service");
		return -1;
	}
	service->gatt_service.attr_count = 0;

	err = ble_service_add_primary_service(service);
	if (err < 0) {
		LOG_ERR("Failed to add item attributes");
		return -1;
	}

	// TODO add any other controls, statistics and logs

	ble_service_init_chrc(service);
	ble_service_init_ccc(service);

	err = ble_service_add_item_attributes(service);
	if (err < 0) {
		LOG_ERR("Failed to add item attributes");
		return -1;
	}

	LOG_INF("Registering %d attributes", service->gatt_service.attr_count);

	err = bt_gatt_service_register(&service->gatt_service);
	if (err < 0) {
		LOG_ERR("Failed to register service");
		return -1;
	}

	LOG_INF("Registering nus_log service");
	err = bt_gatt_service_register(&nus_log_service);


	return 0;
}

void ble_service_shutdown(struct ble_service *service) {
	// TODO
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		printk("Connection failed (err 0x%02x)\n", err);
	} else {
		printk("Connected. MTU size: %d\n", bt_gatt_get_mtu(conn));
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	printk("Disconnected (reason 0x%02x)\n", reason);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

static void auth_cancel(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Pairing cancelled: %s\n", addr);
}

static struct bt_conn_auth_cb auth_cb_display = {
	.cancel = auth_cancel,
};

//TODO check
static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_CUSTOM_SERVICE_VAL),
};

void bt_ready(void)
{
	int err;

	LOG_INF("Bluetooth initialized");

	err = bt_le_adv_start(BT_LE_ADV_CONN_NAME, ad, ARRAY_SIZE(ad), NULL, 0);
	
	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return;
	}

	LOG_INF("Advertising successfully started");

	bt_conn_auth_cb_register(&auth_cb_display);
}