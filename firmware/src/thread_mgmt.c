#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/device.h>
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

void ot_joiner_callback(otError error, void *context) {
    struct otInstance *instance = ((struct openthread_context *) context)->instance;
    openthread_api_mutex_lock(context);
    if (error == OT_ERROR_NONE) {
        LOG_INF("Join success");
        otError err = otThreadSetEnabled(instance, true);
        if (err != OT_ERROR_NONE) {
            LOG_ERR("Failed to start the OpenThread (%d)", error);
            goto error;
        } else {
            LOG_INF("Started thread");
            joined = true;
            goto joined;
        }
    } else {
        LOG_ERR("Failed to join: %d", error);
        goto error;
    }

error:
	otIp6SetEnabled(instance, false);
    // Only allow further joins on failure
    k_sem_give(&join_allowed); 

joined:
    openthread_api_mutex_unlock(context);

}

void thread_join_attempt() {
    int ret;
    otError err;

	struct openthread_context *context = openthread_get_default_context();
	if (context == NULL) {
		LOG_ERR("Openthread context not available");
		return;
	}
    
    ret = k_sem_take(&join_allowed, K_NO_WAIT);
    if (ret != 0) {
        if (joined) {
            LOG_DBG("Already joined");
        } else {
            LOG_DBG("Join already in progress");
        }
        return;
    }

    openthread_api_mutex_lock(context);
    struct otInstance *instance = context->instance;

	err = otIp6SetEnabled(context->instance, true);
    if (err != OT_ERROR_NONE) {
        LOG_ERR("Failed to enable OpenThread interface (%d)", err);
        goto error;
    }

    if (otDatasetIsCommissioned(instance)) {
        LOG_INF("OpenThread is already comissioned");
        err = otThreadSetEnabled(instance, true);
        if (err != OT_ERROR_NONE) {
            LOG_ERR("Failed to start OpenThread (%d)", err);
            goto error;
        }
    } else {
        LOG_INF("Starting Joiner");
        err = otJoinerStart(instance, PSKD, NULL, 
        VENDOR_NAME, VENDOR_MODEL, SW_VERSION, VENDOR_DATA, &ot_joiner_callback, context);
        if (err != OT_ERROR_NONE) {
            LOG_ERR("Failed to start Joiner (%d)", err);
            goto error;
        } else {
        LOG_INF("Waiting for join progress");
            goto ok;
        }
    }

error:
    k_sem_give(&join_allowed);

ok:
    openthread_api_mutex_unlock(context);

}

