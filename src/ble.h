#ifndef BLE_HEADER_H
#define BLE_HEADER_H

#include <zephyr/bluetooth/gatt.h>

#include "telegram.h"

struct user_data {
	int item;
	int last_updated;
	union data_value value;
	struct bt_gatt_attr *attr;
};

struct ble_service {
	struct bt_gatt_service gatt_service;
	struct user_data user_data[_ITEM_COUNT];
    struct bt_gatt_chrc chrc[_ITEM_COUNT];
	struct _bt_gatt_ccc ccc[_ITEM_COUNT]; // Tricky to not use internal for dynamic db
};

void bt_ready(void);

int ble_service_init(struct ble_service *service);
void ble_service_shutdown(struct ble_service *service);
int ble_service_update_item(struct ble_service *service, struct data_item *data_item); 

#endif /* BLE_HEADER_H */