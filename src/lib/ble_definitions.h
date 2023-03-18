#ifndef BLE_DEFINITIONS_HEADER_H
#define BLE_DEFINITIONS_HEADER_H

#include "blep1.h"

#define BT_UUID_BLEP1_SERVICE_VAL BT_UUID_128_ENCODE(0xb3b43e2e, 0x1e86, 0x46f5, 0xa156, 0x4ffd0c7c13ac)

extern struct bt_uuid_128 blep1_uuid;

extern const struct bt_uuid_128 METER_ACTIVE_ENERGY_IN_UUID;
extern const struct bt_uuid_128 METER_ACTIVE_ENERGY_OUT_UUID;
extern const struct bt_uuid_128 ACTIVE_ENERGY_IN_UUID;
extern const struct bt_uuid_128 ACTIVE_ENERGY_OUT_UUID;

struct gatt_ch {
    enum Item item;
    const struct bt_uuid_128 *uuid;
    const struct bt_gatt_cpf *cpf;
    char *description;
};

extern const struct gatt_ch gatt_ch_table[];

#endif /* BLE_DEFINITIONS_HEADER_H */