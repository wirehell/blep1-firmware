#ifndef INPUT_HEADER_H
#define INPUT_HEADER_H


typedef void (*button_event_cb)();

struct button_cb {
    button_event_cb on_button_pressed;
    button_event_cb on_button_released;
    button_event_cb on_button_short_press_released;
    button_event_cb on_button_long_press_released;
    button_event_cb on_button_long_press_detected;
};

int input_init(struct button_cb *callbacks);


#endif /* INPUT_HEADER_H */