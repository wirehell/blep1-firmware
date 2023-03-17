#ifndef PARSER_TASK_HEADER_H
#define PARSER_TASK_HEADER_H

#include "parser.h"

int parser_task_init(struct k_fifo *telegram_rx_queue, struct k_msgq *queue);

#endif /* PARSER_TASK_HEADER_H */