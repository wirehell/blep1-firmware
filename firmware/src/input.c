#include "input.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/device.h>
#include <zephyr/sys/time_units.h>

#if CONFIG_GPIO

LOG_MODULE_REGISTER(input, LOG_LEVEL_DBG);

//static const struct device *const button = DEVICE_DT_GET(DT_NODELABEL(button0));
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);

/* For zephyr 3.4.0, with input system
static void test_cb(struct input_event *evt) {
    LOG_INF("Button press, type: %d, code: %d, value: %d", evt->type, evt->code, evt->value);
}
//INPUT_LISTENER_CB_DEFINE(button, test_cb);
*/

static struct button_cb *button_callbacks;

static void call_if_set(button_event_cb cb) {
    if (cb != NULL) {
        cb();
    }
}

static void on_button_pressed(struct k_work *work) {
    LOG_DBG("Button pressed");
	call_if_set(button_callbacks->on_button_pressed);
}

static void on_button_released(struct k_work *work) {
    LOG_DBG("Button released");
	call_if_set(button_callbacks->on_button_released);
}

static void on_button_short_press_released(struct k_work *work) {
    LOG_DBG("Button short press released");
	call_if_set(button_callbacks->on_button_short_press_released);
}

static void on_button_long_press_released(struct k_work *work) {
    LOG_DBG("Button long press released");
	call_if_set(button_callbacks->on_button_long_press_released);
}

static void on_button_long_press_detected(struct k_work *work) {
    LOG_DBG("Button long press detected");
	call_if_set(button_callbacks->on_button_long_press_detected);
}

K_WORK_DEFINE(button_pressed_work, on_button_pressed);
K_WORK_DEFINE(button_released_work, on_button_released);
K_WORK_DEFINE(button_short_press_released_work, on_button_short_press_released);
K_WORK_DEFINE(button_long_press_released_work, on_button_long_press_released);
K_WORK_DEFINE(button_long_press_detected_work, on_button_long_press_detected);


#define LONG_PRESS_TIME_MS 3000
#define SHORT_PRESS_TIME_MS 50

void press_timer_handler(struct k_timer *ignored) {
    k_work_submit(&button_long_press_detected_work);
}

K_TIMER_DEFINE(press_timer, press_timer_handler, NULL);

static void button_cb(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
    int level = gpio_pin_get_dt(&button);
    if (level != 0 ) {
        // Button pressed
        k_timer_start(&press_timer, K_MSEC(LONG_PRESS_TIME_MS), K_NO_WAIT);
        k_work_submit(&button_pressed_work);
    } else {
        // Button released
        k_ticks_t remaining_ticks = k_timer_remaining_ticks(&press_timer);
        k_timer_stop(&press_timer);
        uint32_t elapsed_time_ms = LONG_PRESS_TIME_MS - k_ticks_to_ms_floor32 (remaining_ticks);
        if (k_timer_status_get(&press_timer) > 0) {
            // Timer expired -> long press
            k_work_submit(&button_long_press_released_work);
        } else if (elapsed_time_ms > SHORT_PRESS_TIME_MS) {
            k_work_submit(&button_short_press_released_work);
        }
        // In any case, signal button relased
        k_work_submit(&button_released_work);
    }
}

static struct gpio_callback button_cb_data;

int input_init(struct button_cb *callbacks) {
    int ret;
    LOG_INF("Initializing input");
    if (!device_is_ready(button.port)) {
		LOG_ERR("%s: device not ready.\n", button.port->name);
		return -1;
	}
    ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
	if (ret != 0) {
		LOG_ERR("Error %d: failed to configure gpio on %s pin %d\n",
			ret, button.port->name, button.pin);
		return -1;
	}

	ret = gpio_pin_interrupt_configure_dt(&button, GPIO_INT_TRIG_BOTH);
	if (ret != 0) {
		LOG_ERR("Error %d: failed to configure interrupt on %s pin %d\n",
			ret, button.port->name, button.pin);
		return -1;
	}
	gpio_init_callback(&button_cb_data, button_cb, BIT(button.pin));
    ret = gpio_add_callback(button.port, &button_cb_data);
    if (ret != 0) {
		LOG_ERR("Failed to add callback");
		return -1;
    }

    if (callbacks == NULL) {
		LOG_ERR("No callbacks provided");
        return -1;
    }
    button_callbacks = callbacks;
    LOG_INF("Initialized input");

    return 0;
}

#endif