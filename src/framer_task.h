#ifndef FRAMER_TASK_HEADER_H
#define FRAMER_TASK_HEADER_H

#include <zephyr/kernel.h>

int framer_task_init(struct k_pipe *input, struct k_fifo *output); 

#endif /* FRAMER_TASK_HEADER_H */