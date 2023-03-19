#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

#include "ble_definitions.h"
#include "blep1.h"
#include "common.h"

struct bt_uuid_128 blep1_uuid = BT_UUID_INIT_128(BT_UUID_BLEP1_SERVICE_VAL);

#define CPF_FORMAT_UINT8 0x04
#define CPF_FORMAT_UINT32 0x08

#define MAX_ATTRIBUTES 512


// We could use 0x2a08 characteristic, but this field is not very useful, 
// so for now, let's just pass it as a fixed length string.
const struct bt_uuid_128 DATE_TIME_UUID = BT_UUID_INIT_128(
	BT_UUID_128_ENCODE(0x769b7853, 0xe490, 0x48e8, 0x895c, 0x1411746cefcc));

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

*/

#define CPF_UNIT_ENERGY_KWH 0x27AB
#define CPF_UNIT_POWER_W 0x2726

#define CPF_FORMAT_UINT8 0x04
#define CPF_FORMAT_UINT32 0x08

#define CPF_FORMAT_UTF8 0x19

struct bt_gatt_cpf CPF_STRING = {
  .format = CPF_FORMAT_UTF8,
};

struct bt_gatt_cpf CPF_UINT32_KWH_FROM_8_3_KWH = {
  .format = CPF_FORMAT_UINT32,
  .exponent = -3,
  .unit = CPF_UNIT_ENERGY_KWH,
};

struct bt_gatt_cpf CPF_UINT32_KVARH_FROM_8_3_KVARH = {
  .format = CPF_FORMAT_UINT32,
  .exponent = -3,
  // No suitable unit exists
};

struct bt_gatt_cpf CPF_UINT32_W_FROM_4_3_KW = {
  .format = CPF_FORMAT_UINT32,
  .exponent = 0,
  .unit = CPF_UNIT_POWER_W,
};

struct bt_gatt_cpf CPF_UINT32_VA_FROM_4_3_KVAR = {
  .format = CPF_FORMAT_UINT32,
  .exponent = 0,
  // No suitable unit exists
};

const struct gatt_ch gatt_ch_table[] = {
	  { DATE_TIME,					        &DATE_TIME_UUID,					        &CPF_STRING,						          "Date and time"},
    { METER_ACTIVE_ENERGY_IN,   	&METER_ACTIVE_ENERGY_IN_UUID,   	&CPF_UINT32_KWH_FROM_8_3_KWH, 		"Meter - Active energy in"},
    { METER_ACTIVE_ENERGY_OUT,  	&METER_ACTIVE_ENERGY_OUT_UUID,  	&CPF_UINT32_KWH_FROM_8_3_KWH, 		"Meter - Active energy out"},
    { METER_REACTIVE_ENERGY_IN,   &METER_REACTIVE_ENERGY_IN_UUID,   &CPF_UINT32_KVARH_FROM_8_3_KVARH, "Meter - Reactive energy in"},
    { METER_REACTIVE_ENERGY_OUT,  &METER_REACTIVE_ENERGY_OUT_UUID,  &CPF_UINT32_KVARH_FROM_8_3_KVARH, "Meter - Reactive energy out"},
    { ACTIVE_ENERGY_IN,         	&ACTIVE_ENERGY_IN_UUID,         	&CPF_UINT32_W_FROM_4_3_KW,    		"Active energy in"},
    { ACTIVE_ENERGY_OUT,        	&ACTIVE_ENERGY_OUT_UUID,        	&CPF_UINT32_W_FROM_4_3_KW,    		"Active energy out"},
    { REACTIVE_ENERGY_IN,         &REACTIVE_ENERGY_IN_UUID,         &CPF_UINT32_VA_FROM_4_3_KVAR,    	"Reactive energy in"},
    { REACTIVE_ENERGY_OUT,        &REACTIVE_ENERGY_OUT_UUID,        &CPF_UINT32_VA_FROM_4_3_KVAR,    	"Reactive energy out"},
};
