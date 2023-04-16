#include "state_indicator.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>

LOG_MODULE_REGISTER(state_indicator, LOG_LEVEL_DBG);

#define STACKSIZE 512
#define PRIORITY 5

struct rgb_color {
    u_int8_t red;
    u_int8_t green;
    u_int8_t blue;
};

struct pulse_pattern {
    int on_ms;
    int interval;
};

struct state_indication {
    const struct rgb_color *color;
    const struct pulse_pattern *pattern;
};

const struct rgb_color MAGENTA =    { .red = 0xff , .green = 0x00, .blue = 0xff };
const struct rgb_color RED =        { .red = 0xff , .green = 0x00, .blue = 0x00 };
const struct rgb_color GREEN =      { .red = 0x00 , .green = 0xff, .blue = 0x00 };
const struct rgb_color YELLOW =     { .red = 0xff , .green = 0xff, .blue = 0x00 };
const struct rgb_color BLUE =       { .red = 0x00 , .green = 0xff, .blue = 0xff };
const struct rgb_color WHITE =      { .red = 0xff , .green = 0xff, .blue = 0xff };
const struct rgb_color CYAN =       { .red = 0x00 , .green = 0xff, .blue = 0xff };
const struct rgb_color BLACK =      { .red = 0x00 , .green = 0x00, .blue = 0x00 };

const struct pulse_pattern ON =     { .on_ms = 1000,    .interval = 1000    };
const struct pulse_pattern OFF =    { .on_ms = 0,       .interval = 1000    };
const struct pulse_pattern SLOW =   { .on_ms = 300,     .interval = 2000    };
const struct pulse_pattern MEDIUM = { .on_ms = 100,     .interval = 500     };
const struct pulse_pattern FAST =   { .on_ms = 50,      .interval = 200     };

static const struct state_indication indications[__NUM_STATES] = {
    { .color = &MAGENTA,    .pattern = &ON },
    { .color = &RED,        .pattern = &ON },
    { .color = &CYAN,       .pattern = &ON },
    { .color = &YELLOW,     .pattern = &FAST },
    { .color = &YELLOW,     .pattern = &SLOW },
    { .color = &GREEN,      .pattern = &MEDIUM },
    { .color = &BLUE,       .pattern = &SLOW },
    { .color = &WHITE,      .pattern = &SLOW },
};

K_SEM_DEFINE(state_updatable, 1, 1);
K_SEM_DEFINE(state_updated, 0, 1);

static struct state_indicator {
    enum indicator_state current_state;
} state_indicator;

#if CONFIG_PWM
static const struct pwm_dt_spec red_pwm_led = PWM_DT_SPEC_GET(DT_ALIAS(red_pwm_led));
static const struct pwm_dt_spec green_pwm_led = PWM_DT_SPEC_GET(DT_ALIAS(green_pwm_led));
static const struct pwm_dt_spec blue_pwm_led = PWM_DT_SPEC_GET(DT_ALIAS(blue_pwm_led));
#endif

#if CONFIG_GPIO
static const struct gpio_dt_spec telegram_notify_led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
#endif

void set_color(const struct rgb_color *color) {
    LOG_DBG("Setting color. r: %x, g: %x, b: %x", color->red, color->green, color->blue);
#if CONFIG_PWM
    pwm_set_pulse_dt(&red_pwm_led, color->red * 1000);
    pwm_set_pulse_dt(&green_pwm_led, color->green * 1000);
    pwm_set_pulse_dt(&blue_pwm_led, color->blue * 1000);
#endif
}

static void pulse_stop(struct k_timer *timer) {
    set_color(&BLACK);
}

K_TIMER_DEFINE(pulse_stop_timer, pulse_stop, NULL);

static void pulse_start(struct k_timer *timer) {
    const struct state_indication *indication = &indications[state_indicator.current_state];
    const struct pulse_pattern *pattern = indication->pattern;
    const struct rgb_color *color = indication->color;
    set_color(color);
    if (pattern->on_ms < pattern->interval) {
        k_timer_start(&pulse_stop_timer, K_MSEC(pattern->on_ms), K_NO_WAIT);
    }
}

K_TIMER_DEFINE(pulse_start_timer, pulse_start, NULL);
static void state_indicator_entry(void *, void *, void *) {
    int ret;
    state_indicator.current_state = INIT;
#if CONFIG_PWM
   	if (!device_is_ready(red_pwm_led.dev) ||
	    !device_is_ready(green_pwm_led.dev) ||
	    !device_is_ready(blue_pwm_led.dev)) {
        LOG_ERR("PWM leds not ready");
		return;
	} 
#endif
#if CONFIG_GPIO
    if (!device_is_ready(telegram_notify_led.port)) {
        LOG_ERR("led0 not ready");
        return;
    }
    ret = gpio_pin_configure_dt(&telegram_notify_led, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
        LOG_ERR("Could not configure led0");
		return;
	}
#endif

    while (true) {
        LOG_DBG("State updated, reloading timers");
        k_timer_stop(&pulse_start_timer);
        k_timer_stop(&pulse_stop_timer);
        const struct state_indication *indication = &indications[state_indicator.current_state];
        const struct pulse_pattern *pattern = indication->pattern;
        k_timer_start(&pulse_start_timer, K_NO_WAIT, K_MSEC(pattern->interval));
        k_sem_give(&state_updatable);
        k_sem_take(&state_updated, K_FOREVER);
    }
}

void state_indicator_set_state(enum indicator_state state) {
    int ret = k_sem_take(&state_updatable, K_MSEC(1000));
    if (ret < 0) {
        LOG_ERR("Could not update state. State: %d, Error: %d", state, ret);
        return;
    }
    LOG_INF("Changing from state: %d, to %d", state_indicator.current_state, state);
    state_indicator.current_state = state;
    k_sem_give(&state_updated);
}

#define TELEGRAM_NOTIFICATION_MS 500

static void telegram_notify_stop(struct k_timer *timer) {
    LOG_DBG("Telegram arrived notifiaction stop");
#if CONFIG_GPIO
    gpio_pin_set_dt(&telegram_notify_led, 0);
#endif
}

K_TIMER_DEFINE(telegram_notify_timer, telegram_notify_stop, NULL);

void state_indicator_notify_telegram() {
    k_timer_start(&telegram_notify_timer, K_MSEC(TELEGRAM_NOTIFICATION_MS), K_NO_WAIT);
    LOG_DBG("Telegram arrived notifiaction start");
#if CONFIG_GPIO
    gpio_pin_set_dt(&telegram_notify_led, 1);
#endif
}

K_THREAD_DEFINE(state_indicator_task, STACKSIZE,
                state_indicator_entry, NULL, NULL, NULL,
                PRIORITY, 0, 0);
