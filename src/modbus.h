#ifndef MODBUS_H
#define MODBUS_H

#include "lib/blep1.h"
#include "lib/telegram.h"
#include "lib/value_store.h"

// Map Items to DATA_BASE_ADDRESS + item number * 32
#define DATA_BASE_ADDRESS 0x0800

int modbus_init(struct value_store *store);

#endif /* MODBUS_H */