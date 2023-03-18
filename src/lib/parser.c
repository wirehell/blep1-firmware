#include <regex.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/buf.h>
#include <zephyr/sys/crc.h>
#include <stdlib.h>
#include <sys/types.h>
#include <errno.h>

#include "parser.h"
#include "common.h"
#include "telegram.h"
#include "blep1.h"

LOG_MODULE_REGISTER(parser, LOG_LEVEL_DBG);

#define HEADER_REGEX_PATTERN "^\\/[A-Za-z]{3}.(.+)$"
#define DATA_LINE_REGEX_PATTERN "^([^\\(]+)\\(([^\\*]+)(\\*.*)?\\)$"
#define DATA_REGEX_DOUBLE_LONG_UNSIGNED_8_3 "^([0-9]{8})\\.([0-9]{3})$"
#define DATA_REGEX_DOUBLE_LONG_UNSIGNED_4_3 "^([0-9]{4})\\.([0-9]{3})$"
#define DATA_REGEX_DATE_TIME_STRING "^([0-9]{12}W|S)$"

#define UNIT_REGEX_K_WATT_HOUR "^(kwh)|(kWh)$"
#define UNIT_REGEX_K_WATT "^(kw)|(kW)$"
#define UNIT_REGEX_K_VOLT_AMPERE_HOUR_REACTIVE "^(kvarh)|(kVarh)|(kVArh)|(kVARh)$"
#define UNIT_REGEX_K_VOLT_AMPERE_REACTIVE "^(kvar)|(kVar)|(kVAr)|(kVAR)$"
#define UNIT_REGEX_VOLT "^V$"
#define UNIT_REGEX_AMPERE "^A$"


void parser_free(struct parser *parser) {
    regfree(&parser->header_regex);
    regfree(&parser->data_line_regex);
    regfree(&parser->date_time_regex);
    regfree(&parser->double_long_unsigned_8_3_regex);
    regfree(&parser->double_long_unsigned_4_3_regex);
    regfree(&parser->unit_kwh_regex);
    regfree(&parser->unit_kw_regex);
    regfree(&parser->unit_kvarh_regex);
    regfree(&parser->unit_kvar_regex);
    regfree(&parser->unit_volt_regex);
    regfree(&parser->unit_ampere_regex);
    common_heap_free(parser);
}

struct parser * parser_init() {
    struct parser *parser = common_heap_alloc(sizeof(struct parser));
    if (parser == NULL) {
        LOG_ERR("Could not allocate parser");
        return NULL;
    }

    LOG_DBG("Initializing parser");
    int reti;

    reti = regcomp(&(parser->header_regex), HEADER_REGEX_PATTERN, REG_EXTENDED);
    if (reti) {
        LOG_ERR("Could not compile header regex");
        return NULL;
    }
    reti = regcomp(&parser->data_line_regex, DATA_LINE_REGEX_PATTERN, REG_EXTENDED);
    if (reti) {
        LOG_ERR("Could not compile header regex");
        return NULL;
    }
    reti = regcomp(&parser->date_time_regex, DATA_REGEX_DATE_TIME_STRING, REG_EXTENDED);
    if (reti) {
        LOG_ERR("Could not compile regex for date_time");
        return NULL;
    }
    reti = regcomp(&parser->double_long_unsigned_8_3_regex, DATA_REGEX_DOUBLE_LONG_UNSIGNED_8_3, REG_EXTENDED);
    if (reti) {
        LOG_ERR("Could not compile regex for double-long-unsigned-8-3");
        return NULL;
    }
    reti = regcomp(&parser->double_long_unsigned_4_3_regex, DATA_REGEX_DOUBLE_LONG_UNSIGNED_4_3, REG_EXTENDED);
    if (reti) {
        LOG_ERR("Could not compile regex for double-long-unsigned-4-3");
        return NULL;
    }
    reti = regcomp(&parser->unit_kwh_regex, UNIT_REGEX_K_WATT_HOUR, REG_EXTENDED);
    if (reti) {
        LOG_ERR("Could not compile regex for kWH");
        return NULL;
    }
    reti = regcomp(&parser->unit_kw_regex, UNIT_REGEX_K_WATT, REG_EXTENDED);
    if (reti) {
        LOG_ERR("Could not compile regex for kW");
        return NULL;
    }
    reti = regcomp(&parser->unit_kvarh_regex, UNIT_REGEX_K_VOLT_AMPERE_HOUR_REACTIVE, REG_EXTENDED);
    if (reti) {
        LOG_ERR("Could not compile regex for kvarh");
        return NULL;
    }
    reti = regcomp(&parser->unit_kvar_regex, UNIT_REGEX_K_VOLT_AMPERE_REACTIVE, REG_EXTENDED);
    if (reti) {
        LOG_ERR("Could not compile regex for kvar");
        return NULL;
    }
    reti = regcomp(&parser->unit_volt_regex, UNIT_REGEX_VOLT, REG_EXTENDED);
    if (reti) {
        LOG_ERR("Could not compile regex for kvar");
        return NULL;
    }
    reti = regcomp(&parser->unit_ampere_regex, UNIT_REGEX_AMPERE, REG_EXTENDED);
    if (reti) {
        LOG_ERR("Could not compile regex for kvar");
        return NULL;
    }

    LOG_INF("Initialized parser");
    return parser;
}



uint8_t * parse_header(struct parser *parser, char *line) {
    regmatch_t pmatch[2];
    int err = regexec(&parser->header_regex, line, 2, pmatch, 0);
    LOG_DBG("Parsed header: %d", err);
    LOG_DBG("Parsed header: %s", line);
    if (err != 0) {
        LOG_ERR("Failed to parse header (%d) from: %s", err, line);
        return NULL;
    }
    if (pmatch[1].rm_eo < 0 || pmatch[1].rm_so < 0) {
        LOG_ERR("Negative offsets, %d, %d", pmatch[1].rm_so, pmatch[1].rm_eo); // Some bug hitting native_posix;
        return NULL;
    }
    int len = pmatch[1].rm_eo - pmatch[1].rm_so;
    uint8_t *src = &line[pmatch[1].rm_so];
    uint8_t *identifier = common_heap_alloc(sizeof(uint8_t) * (len + 1));
    if (identifier == NULL) {
        LOG_ERR("Failed to allocate identifier buffer");
        return NULL;
    }
    memcpy(identifier, src , len);
    identifier[len] = '\0';
    return identifier;
}

const struct data_definition * parse_obis(struct parser *parser, char *obis) {
    // Linear search is good enough for now
    for (int i = 0 ; i < _ITEM_COUNT ; i++) {
        if (strcmp(obis, data_definition_table[i].obis) == 0) {
            return &data_definition_table[i];
        }
    }
    return NULL;
}

static uint32_t parse_double_long_unsigned(struct parser *parser, regex_t *re, char *data) {
    int err;
    char *start, *end;
    regmatch_t pmatch[3];

    err = regexec(re, data, 3, pmatch, 0);
    if (err != 0) {
	    LOG_ERR("Failed to parse double-long-unsigned: %s", data);
        errno = EINVAL;
	    return 0;
    }

    start = data;
    end = &data[pmatch[1].rm_eo];
    uint32_t integer = strtoul(start, &end, 10);

    start = &data[pmatch[2].rm_so];
    end = &data[pmatch[2].rm_eo];
    uint32_t fraction = strtoul(start, &end, 10);

    // TODO check bounds do errno

    return 1000 * integer + fraction;

}

static void parse_date_time_into(struct parser *parser, char *data, struct data_item *data_item) {
    int err;
    char *start, *end;
    regmatch_t pmatch[3];

    err = regexec(&parser->date_time_regex, data, 3, pmatch, 0);
    if (err != 0) {
	    LOG_ERR("Failed to parse date_time string: %s", data);
        errno = EINVAL;
	    return;
    }

    start = data;
    end = &data[pmatch[1].rm_eo];
    memcpy(data_item->value.date_time, start, 13);
    data_item->value.date_time[13] = '\0';

}

static uint32_t parse_double_long_unsigned_8_3(struct parser *parser, char *data) {
    return parse_double_long_unsigned(parser, &parser->double_long_unsigned_8_3_regex, data);
}

static uint32_t parse_double_long_unsigned_4_3(struct parser *parser, char *data) {
    return parse_double_long_unsigned(parser, &parser->double_long_unsigned_4_3_regex, data);
}

int parse_value_into(struct parser *parser, struct data_item *data_item, char *data, enum Format format) {
    errno = 0;
    switch(format) {
        case DATE_TIME_STRING:
            parse_date_time_into(parser, data, data_item);
            LOG_DBG("Parsed datetime: %s", data_item->value.date_time);
            break;
        case DOUBLE_LONG_UNSIGNED_8_3: 
            data_item->value.double_long_unsigned = parse_double_long_unsigned_8_3(parser, data);
            LOG_DBG("Parsed double-long-unsigned-8-3: %u", data_item->value.double_long_unsigned);
            break;
        case DOUBLE_LONG_UNSIGNED_4_3: 
            data_item->value.double_long_unsigned = parse_double_long_unsigned_4_3(parser, data);
            LOG_DBG("Parsed double-long-unsigned-4-3: %u", data_item->value.double_long_unsigned);
            break;
        
        default:
        LOG_ERR("Format not implemented: %d", format);
        return -1;

    }
    if (errno != 0) {
        LOG_ERR("Errno(%d) for format:(%d): %s, value: %s", errno, format, strerror(errno), data);
        return -1;
    }
    return 0;
}

regex_t * parser_regex_for_unit(struct parser *parser, enum Unit unit) {
    switch(unit) {
        case K_WATT_HOUR:                   return &parser->unit_kwh_regex; 
        case K_VOLT_AMPERE_HOUR_REACTIVE:   return &parser->unit_kvarh_regex; 
        case K_WATT:                        return &parser->unit_kw_regex;
        case K_VOLT_AMPERE_REACTIVE:        return &parser->unit_kvar_regex;
        case VOLT:                          return &parser->unit_volt_regex;
        case AMPERE:                        return &parser->unit_ampere_regex;
        default:
    }
    LOG_ERR("Not implemented unit: %d", unit);
    return NULL;
}

int check_unit(struct parser *parser, char *unit, enum Unit expected_unit) {
    if (expected_unit == NONE) {
        if (unit != NULL) {
            LOG_ERR("Did not expect unit, but recived one (%s)", unit);
            return -1;
        }
        return 0;
    }

    if (unit == NULL) {
        LOG_ERR("Expected a unit (%d), but recived none", unit);
        return -1;
    }

    regex_t *unit_regex = parser_regex_for_unit(parser, expected_unit);
    if (unit_regex == NULL) {
        return -1;
    }
    int err = regexec(unit_regex, unit, 0, NULL, 0);
    if (err != 0) {
        LOG_ERR("Unit regex failed, err: %d unit: %d, string: %s", err, expected_unit, unit);
        return -1;
    }
    return 0;
}

int parse_value(struct parser *parser, struct data_item *data_item, const struct data_definition *def, char *data, char *unit) {
    if (check_unit(parser, unit, def->unit) < 0) {
        return -1;
    }
    data_item->item = def->item;
    int err = parse_value_into(parser, data_item, data, def->format);
    if (err < 0) {
        LOG_ERR("Could not parse value");
        return -1; 
    }
    return 0;
}

int parse_data_line(struct parser *parser, struct data_item *data_item, char *line) {
    regmatch_t pmatch[4];
    int err;
    err = regexec(&parser->data_line_regex, line, 4, pmatch, 0);
    if (err != 0) {
        LOG_ERR("Failed to parse dataline (%d) from: %s", err, line);
        return -1;
    }
    // strtok_r is already destructive, these are safe from indexing perspective
    line[pmatch[1].rm_eo] = '\0';
    line[pmatch[2].rm_eo] = '\0';
    char *obis = &line[pmatch[1].rm_so];
    char *data = &line[pmatch[2].rm_so];
    char *unit;
    if (pmatch[3].rm_so < 0) {
        unit = NULL;
    } else {
        unit = &line[pmatch[3].rm_so + 1]; // discard *
        line[pmatch[3].rm_eo] = '\0';
    }

    LOG_DBG("Parsed data line, obis: %s data: %s", obis, data);

    const struct data_definition *def = parse_obis(parser, obis);
    if (def == NULL) {
        LOG_WRN("Unknown obis: %s", obis);
        return 1;
    }
    err = parse_value(parser, data_item, def, data, unit);
    if (err < 0) {
        LOG_WRN("Could not parse data");
        return -1;
    }

    return 0;
}

struct telegram * parse_telegram(struct parser *parser, struct net_buf *telegram_buf) {
    struct telegram *telegram = telegram_init();
    if (telegram == NULL) {
        return NULL;
    }
    char *line;
    char *rest = telegram_buf->data;
    line = strtok_r(rest, "\r\n", &rest);
    if (line == NULL) {
        LOG_INF("First line not found");
        goto failure;
    }
    int err;
    uint8_t *identifier = parse_header(parser, line);
    if (identifier == NULL) {
        LOG_WRN("Could not parse header");
        goto failure;
    }
    telegram->identifier = identifier;

    while ((line = strtok_r(rest, "\r\n", &rest))) {
        struct data_item data_item;
        err = parse_data_line(parser, &data_item, line);
        if (err < 0) {
            goto failure;
        } 
        if (err == 0) {
            telegram_item_append(telegram, &data_item);
        } else {
            // If err > 0 we can continue
        }
    }

    return telegram;

    failure:
    telegram_free(telegram);
    return NULL;
}
