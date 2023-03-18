#ifndef PARSER_HEADER_H
#define PARSER_HEADER_H

#include <regex.h>
#include <zephyr/net/buf.h>
#include "blep1.h"
#include "telegram.h"

struct parser {
    regex_t header_regex;
    regex_t footer_regex;
    regex_t data_line_regex;
    regex_t date_time_regex;
    regex_t double_long_unsigned_8_3_regex;
    regex_t double_long_unsigned_4_3_regex;
    regex_t long_unsigned_3_1_regex;
    regex_t long_signed_3_1_regex;
    regex_t unit_kwh_regex;
    regex_t unit_kw_regex;
    regex_t unit_kvarh_regex;
    regex_t unit_kvar_regex;
    regex_t unit_volt_regex;
    regex_t unit_ampere_regex;
};

struct parser * parser_init();
void parser_free(struct parser *parser);
 
uint8_t * parse_header(struct parser *parser, char *line);
int parse_footer(struct parser *parser, char *line);

int parse_data_line(struct parser *parser, struct data_item *data_item, char *line);

struct telegram * parse_telegram(struct parser *parser, struct net_buf *telegram_buf); 
bool checksum_is_ok(struct net_buf *telegram_buf);

#endif /* PARSER_HEADER_H */