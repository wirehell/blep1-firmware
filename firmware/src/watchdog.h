#ifndef WATCHDOG_HEADER_H
#define WATCHDOG_HEADER_H

enum watchdog {
	MODBUS_READ,
	TELEGRAM_RECEIVED,
	__NUM_DOGS,
};

int watchdog_init();
void watchdog_feed(enum watchdog dog);

#endif /* WATCHDOG_HEADER_H */