#ifndef STATE_INDICATOR_H
#define STATE_INDICATOR_H

#include <sys/types.h>
#include <stdbool.h>


enum indicator_state {
    INIT,
    ERROR,
    STARTED,
    NOT_COMISSIONED,
    JOINING_NETWORK,
    JOINING_FAILED,
    AWAITING_ROUTES,
    CONNECTED,
    ACTIVE,
    __NUM_STATES,
};

void state_indicator_set_state(enum indicator_state state);
void state_indicator_notify_telegram();


#endif /* STATE_INDICATOR_H */