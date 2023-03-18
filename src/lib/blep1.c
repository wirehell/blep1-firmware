#include "blep1.h"

#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

const struct data_definition data_definition_table[] = {
    { DATE_TIME,                    "0-0:1.0.0", DATE_TIME_STRING,         NONE },
    { METER_ACTIVE_ENERGY_IN,       "1-0:1.8.0", DOUBLE_LONG_UNSIGNED_8_3, K_WATT_HOUR },
    { METER_ACTIVE_ENERGY_OUT,      "1-0:2.8.0", DOUBLE_LONG_UNSIGNED_8_3, K_WATT_HOUR },
    { METER_REACTIVE_ENERGY_IN,     "1-0:3.8.0", DOUBLE_LONG_UNSIGNED_8_3, K_VOLT_AMPERE_HOUR_REACTIVE },
    { METER_REACTIVE_ENERGY_OUT,    "1-0:4.8.0", DOUBLE_LONG_UNSIGNED_8_3, K_VOLT_AMPERE_HOUR_REACTIVE },
    { ACTIVE_ENERGY_IN,             "1-0:1.7.0", DOUBLE_LONG_UNSIGNED_4_3, K_WATT },
    { ACTIVE_ENERGY_OUT,            "1-0:2.7.0", DOUBLE_LONG_UNSIGNED_4_3, K_WATT },
    { REACTIVE_ENERGY_IN,           "1-0:3.7.0", DOUBLE_LONG_UNSIGNED_4_3, K_VOLT_AMPERE_REACTIVE },
    { REACTIVE_ENERGY_OUT,          "1-0:4.7.0", DOUBLE_LONG_UNSIGNED_4_3, K_VOLT_AMPERE_REACTIVE },
};
