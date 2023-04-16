#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/device.h>

#include "state_indicator.h"

#if CONFIG_OPENTHREAD

#include <zephyr/net/openthread.h>
#include <openthread/dataset.h>
#include <openthread/instance.h>
#include <openthread/joiner.h>
#include <openthread/thread.h>

// Todo move to config
const char PSKD[] ="ME0W4ME";
const char VENDOR_NAME[] = "wirehell";
const char VENDOR_MODEL[] = "OpenP1";
const char SW_VERSION[] = "1.0";
const char VENDOR_DATA[] = "";

LOG_MODULE_REGISTER(thread_mgmt, LOG_LEVEL_DBG);

K_SEM_DEFINE(join_allowed, 1, 1);

static bool joined = false;

void thread_factory_reset() {
    LOG_INF("Performing factory reset");
	struct openthread_context *context = openthread_get_default_context();
	if (context == NULL) {
		LOG_ERR("Factory reset failed. Openthread context not available");
		return;
	}
    openthread_api_mutex_lock(context);
    otInstanceFactoryReset(context->instance);
    openthread_api_mutex_unlock(context);
    return;
}

static void on_join() {
    LOG_INF("Thread network is up");
    joined = true;
    state_indicator_set_state(AWAITING_ROUTES);
}

static void ot_joiner_callback(otError error, void *context) {
    struct otInstance *instance = ((struct openthread_context *) context)->instance;
    openthread_api_mutex_lock(context);
    if (error == OT_ERROR_NONE) {
        LOG_INF("Join success");
        otError err = otThreadSetEnabled(instance, true);
        if (err != OT_ERROR_NONE) {
            LOG_ERR("Failed to start the OpenThread (%d)", error);
            goto error;
        } else {
            on_join();
            goto joined;
        }
    } else {
        LOG_ERR("Failed to join: %d", error);
        goto error;
    }

error:
    state_indicator_set_state(JOINING_FAILED);
	otIp6SetEnabled(instance, false);
    // Only allow further joins on failure
    k_sem_give(&join_allowed); 

joined:
    openthread_api_mutex_unlock(context);

}

static int thread_enable_if_comissioned() {
    int ret = -1;
    otError err;

	struct openthread_context *context = openthread_get_default_context();

    openthread_api_mutex_lock(context);
    struct otInstance *instance = context->instance;

	err = otIp6SetEnabled(context->instance, true);
    if (err != OT_ERROR_NONE) {
        LOG_ERR("Failed to enable OpenThread interface (%d)", err);
        ret = -1;
        goto not_up;
    }

    if (!otDatasetIsCommissioned(instance)) {
        LOG_INF("OpenThread is not comissioned");
        goto not_up;
    }

    err = otThreadSetEnabled(instance, true);
    if (err != OT_ERROR_NONE) {
        LOG_ERR("Failed to start OpenThread (%d)", err);
        goto not_up;
    } else {
        on_join();
        ret = 0;
        goto done;
    }

    not_up:
    state_indicator_set_state(NOT_COMISSIONED);

    done:
    openthread_api_mutex_unlock(context);

    return ret;
}

int thread_up() {
    int err; 

    err = k_sem_take(&join_allowed, K_NO_WAIT);
    if (err != 0) {
		LOG_ERR("BUG: Openthread join is not allowed");
        goto error;
    }

    err = thread_enable_if_comissioned();
    if (err < 0) {
        k_sem_give(&join_allowed);
    }

    error:

    return err;
}

void thread_join_attempt() {
    int ret;
    otError err;

    ret = k_sem_take(&join_allowed, K_NO_WAIT);
    if (ret != 0) {
        if (joined) {
            LOG_DBG("Already joined");
        } else {
            LOG_DBG("Join already in progress");
        }
        return;
    }

    if (thread_enable_if_comissioned() == 0) {
        LOG_INF("Already joined");
        return;
    }

	struct openthread_context *context = openthread_get_default_context();
	if (context == NULL) {
		LOG_ERR("Openthread context not available");
		return;
	}

    openthread_api_mutex_lock(context);
    struct otInstance *instance = context->instance;

	err = otIp6SetEnabled(context->instance, true);
    if (err != OT_ERROR_NONE) {
        LOG_ERR("Failed to enable OpenThread interface (%d)", err);
        goto error;
    }

    LOG_INF("Starting Joiner");
    err = otJoinerStart(instance, PSKD, NULL, 
    VENDOR_NAME, VENDOR_MODEL, SW_VERSION, VENDOR_DATA, &ot_joiner_callback, context);
    if (err != OT_ERROR_NONE) {
        LOG_ERR("Failed to start Joiner (%d)", err);
        goto error;
    } else {
        LOG_INF("Waiting for join progress");
        state_indicator_set_state(JOINING_NETWORK);
        goto ok;
    }

error:
    state_indicator_set_state(JOINING_FAILED);
    k_sem_give(&join_allowed);

ok:
    openthread_api_mutex_unlock(context);

}

#endif