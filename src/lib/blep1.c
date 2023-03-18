#include "blep1.h"

#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>


const struct data_definition data_definition_table[] = {
    { METER_ACTIVE_ENERGY_IN,   "1-0:1.8.0", DOUBLE_LONG_UNSIGNED_8_3, "kWh" },
    { METER_ACTIVE_ENERGY_OUT,  "1-0:2.8.0", DOUBLE_LONG_UNSIGNED_8_3, "kWh" },
    { ACTIVE_ENERGY_IN,         "1-0:1.7.0", DOUBLE_LONG_UNSIGNED_4_3, "kW"  },
    { ACTIVE_ENERGY_OUT,        "1-0:2.7.0", DOUBLE_LONG_UNSIGNED_4_3, "kW"  },
};

