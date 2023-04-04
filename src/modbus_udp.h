#ifndef MODBUS_UDP_H
#define MODBUS_UDP_H

#include "lib/blep1.h"
#include "lib/telegram.h"
#include "lib/value_store.h"


// Map Items to DATA_BASE_ADDRESS + item number * 32
#define DATA_BASE_ADDRESS 0x0800

#define BASE_ADDRESS(x) (x << 5);

#define IRB_DATE_TIME_BASE           BASE_ADDRESS(0)
#define IRB_METER_ACTIVE_ENERGY_IN   BASE_ADDRESS(1)
#define IRB_METER_ACTIVE_ENERGY_OUT  BASE_ADDRESS(2)
#define IRB_ACTIVE_ENERGY_IN         BASE_ADRRESS(3)
#define IRB_ACTIVE_ENERGY_OUT        BASE_ADRRESS(4)


struct modbus_udp_server {
};


int modbus_udp_init(struct value_store *store);



#endif /* MODBUS_UDP_H */