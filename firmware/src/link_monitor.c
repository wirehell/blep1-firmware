#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/openthread.h>

#include <openthread/dataset.h>
#include <openthread/instance.h>
#include <openthread/netdata.h>

#include "state_indicator.h"

#if CONFIG_OPENTHREAD

LOG_MODULE_REGISTER(link_monitor, LOG_LEVEL_INF);

#define STACKSIZE 512
#define PRIORITY 7

K_SEM_DEFINE(link_monitor_check, 0, 1);

void link_monitor_event(struct k_timer *timer) {
    k_sem_give(&link_monitor_check);
}

K_TIMER_DEFINE(link_monitor_timer, link_monitor_event, NULL);

static uint8_t prefix_buf[64];

static int is_up() {
    // Consider up if we have any external routes
	struct openthread_context *context = openthread_get_default_context();
    if (context == NULL) {
        return -1;
    }

    openthread_api_mutex_lock(context);
    struct otInstance *instance = context->instance;
  
    otNetworkDataIterator it = OT_NETWORK_DATA_ITERATOR_INIT;
    struct otExternalRouteConfig route_config;
    int route_count = 0;

    while (OT_ERROR_NONE == otNetDataGetNextRoute(instance, &it, &route_config)) {
        route_count++;
        otIp6PrefixToString(&route_config.mPrefix, prefix_buf, sizeof(prefix_buf));
        LOG_DBG("Found external route RLOC16: %d Prefix: %s", route_config.mRloc16, prefix_buf);
    }

    openthread_api_mutex_unlock(context);

    return route_count;
}

void link_monitor_task(void *, void *, void *) {
    k_timer_start(&link_monitor_timer, K_MSEC(1000), K_MSEC(1000));
    while (true) {
        k_sem_take(&link_monitor_check, K_FOREVER);
        enum indicator_state current_state = state_indicator_get_state();
        if (current_state == CONNECTED || current_state == AWAITING_ROUTES) {
            int ret = is_up();
            if (ret > 0 ) {
                state_indicator_set_state(CONNECTED);
            } else if (ret == 0) {
                state_indicator_set_state(AWAITING_ROUTES);
            } else {
                state_indicator_set_state(ERROR);
            } 
        }
    }
}

K_THREAD_DEFINE(link_monitor_thread, STACKSIZE,
                link_monitor_task, NULL, NULL, NULL,
                PRIORITY, 0, 0);


#endif