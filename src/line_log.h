#ifndef LINE_LOG_HEADER_H
#define LINE_LOG_HEADER_H

#define LINE_BUF_SIZE 200

struct line_log {
    char *buf;
    int pos;
};

struct line_log * line_log_init();
void line_log_push(struct line_log *log, char c);
void line_log_reset(struct line_log *log);
void line_log_free(struct line_log *log);
 

#endif /* LINE_LOG_HEADER_H */