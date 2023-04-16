#include "watchdog.h"

#include <zephyr/device.h>
#include <zephyr/logging/log.h>

#if CONFIG_WATCHDOG

#include <zephyr/drivers/watchdog.h>
LOG_MODULE_REGISTER(openp1_watchdog, LOG_LEVEL_DBG);

#define WDT_NODE DT_ALIAS(watchdog0)

const struct device *const hw_wdt_dev = DEVICE_DT_GET(WDT_NODE);

const struct wdt_window window = {
	 .min = 0, .max = 10 * 60 * 1000  
}; 

static int channel_id[__NUM_DOGS];

int watchdog_init() {

    int ret = 0;

	LOG_INF("Initializing watchdog");

	if (!device_is_ready(hw_wdt_dev)) {
		LOG_ERR("Watchdog not ready: %s", hw_wdt_dev->name);
		return -1;
	}

	for (int i = 0 ; i < __NUM_DOGS ; i++) {
		struct wdt_timeout_cfg cfg = {
			.callback = NULL,
			.flags = WDT_FLAG_RESET_SOC,
			.window = window,
		};
		ret = wdt_install_timeout(hw_wdt_dev, &cfg);
		if (ret < 0) {
			LOG_ERR("Watchdog install failure: %d", ret);
			return -1;
		}
		channel_id[i] = ret;
	}

	ret = wdt_setup(hw_wdt_dev, WDT_OPT_PAUSE_HALTED_BY_DBG);
	if (ret != 0) {
		LOG_ERR("Watchdog setup failure: %d", ret);
		return -1;
	}

	return 0;
}

void watchdog_feed(enum watchdog dog) {
	if (dog != __NUM_DOGS) {
		wdt_feed(hw_wdt_dev, channel_id[dog]);
	}
}


#endif