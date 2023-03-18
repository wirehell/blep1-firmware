#include <regex.h>
#include "lib/parser.h"
#include "lib/common.h"
#include "lib/telegram.h"

#include <zephyr/ztest.h>
#include <zephyr/net/buf.h>

NET_BUF_POOL_DEFINE(test_parser_buf_pool, 16, 8192, 0, NULL);

ZTEST_SUITE(parser_suite, NULL, NULL, NULL, NULL, NULL);

ZTEST(parser_suite, test_parse_header)
{
	struct parser *parser = parser_init();
	char *str = "/ASD5id123";
	uint8_t *identifier = parse_header(parser, str);
	zassert_not_null(identifier);
	zassert_equal(strcmp(identifier, "id123"), 0);
	common_heap_free(identifier);
	parser_free(parser);
}

ZTEST(parser_suite, test_data_line_unknown_obis) {
	struct parser *parser = parser_init();
	char str[] = "9-9:9.8.0(00006678.394*kWh)";
	struct data_item data_item;
	zassert_equal(parse_data_line(parser, &data_item, str), 1);
	parser_free(parser);
}

ZTEST(parser_suite, test_data_line_1_7_0) {
	struct parser *parser = parser_init();
	char str[] = "1-0:1.7.0(0002.123*kW)";
	struct data_item data_item;
	zassert_ok(parse_data_line(parser, &data_item, str));
	zassert_equal(data_item.item, ACTIVE_ENERGY_IN);
	zassert_equal(data_item.value.double_long_unsigned, 2123LU);
	parser_free(parser);
}

ZTEST(parser_suite, test_data_line_1_8_0) {
	struct parser *parser = parser_init();
	char str[] = "1-0:1.8.0(00006678.394*kWh)";
	struct data_item data_item;
	zassert_ok(parse_data_line(parser, &data_item, str));
	zassert_equal(data_item.item, METER_ACTIVE_ENERGY_IN);
	zassert_equal(data_item.value.double_long_unsigned, 6678394LU);
	parser_free(parser);
}

ZTEST(parser_suite, test_data_line_2_8_0) {
	struct parser *parser = parser_init();
	char str[] = "1-0:2.8.0(00016678.394*kWh)";
	struct data_item data_item;
	zassert_ok(parse_data_line(parser, &data_item, str));
	zassert_equal(data_item.item, METER_ACTIVE_ENERGY_OUT);
	zassert_equal(data_item.value.double_long_unsigned, 16678394LU);
	parser_free(parser);
}

ZTEST(parser_suite, test_data_line_3_8_0) {
	struct parser *parser = parser_init();
	char str[] = "1-0:3.8.0(00006678.394*kvarh)";
	struct data_item data_item;
	zassert_ok(parse_data_line(parser, &data_item, str));
	zassert_equal(data_item.item, METER_REACTIVE_ENERGY_IN);
	zassert_equal(data_item.value.double_long_unsigned, 6678394LU);
	parser_free(parser);
}

ZTEST(parser_suite, test_data_line_4_8_0) {
	struct parser *parser = parser_init();
	char str[] = "1-0:4.8.0(00006678.394*kvarh)";
	struct data_item data_item;
	zassert_ok(parse_data_line(parser, &data_item, str));
	zassert_equal(data_item.item, METER_REACTIVE_ENERGY_OUT);
	zassert_equal(data_item.value.double_long_unsigned, 6678394LU);
	parser_free(parser);
}

ZTEST(parser_suite, test_data_line_missing_unit) {
	struct parser *parser = parser_init();
	char str[] = "1-0:1.8.0(00006678.394)";
	struct data_item data_item;
	zassert_equal(parse_data_line(parser, &data_item, str), -1);
	parser_free(parser);
}

ZTEST(parser_suite, test_data_line_wrong_unit) {
	struct parser *parser = parser_init();
	char str[] = "1-0:1.8.0(00006678.394*Wh)";
	struct data_item data_item;
	zassert_equal(parse_data_line(parser, &data_item, str), -1);
	parser_free(parser);
}

ZTEST(parser_suite, test_data_line_invalid_number) {
	struct parser *parser = parser_init();
	char str[] = "1-0:1.8.0(0000a678.394*kWh)";
	struct data_item data_item;
	zassert_equal(parse_data_line(parser, &data_item, str), -1);
	parser_free(parser);
}

ZTEST(parser_suite, test_landgyr_e360) {
		char test_data[] = 
		"/LGF5E360\r\n\r\n"
		"0-0:1.0.0(210222161900W)\r\n"
		"1-0:1.8.0(00000896.020*kWh)\r\n"
		"1-0:2.8.0(00000048.792*kWh)\r\n"
		"1-0:3.8.0(00000518.309*kVArh)\r\n"
		"1-0:4.8.0(00000023.732*kVArh)\r\n"
		"1-0:1.7.0(0000.000*kW)\r\n"
		"1-0:2.7.0(0000.020*kW)\r\n"
		"1-0:3.7.0(0000.000*kVAr)\r\n"
		"1-0:4.7.0(0000.308*kVAr)\r\n"
		"1-0:21.7.0(0000.000*kW)\r\n"
		"1-0:22.7.0(0000.012*kW)\r\n"
		"1-0:41.7.0(0000.000*kW)\r\n"
		"1-0:42.7.0(0000.071*kW)\r\n"
		"1-0:61.7.0(0000.063*kW)\r\n"
		"1-0:62.7.0(0000.000*kW)\r\n"
		"1-0:23.7.0(0000.000*kVAr)\r\n"
		"1-0:24.7.0(0000.146*kVAr)\r\n"
		"1-0:43.7.0(0000.000*kVAr)\r\n"
		"1-0:44.7.0(0000.135*kVAr)\r\n"
		"1-0:63.7.0(0000.000*kVAr)\r\n"
		"1-0:64.7.0(0000.026*kVAr)\r\n"
		"1-0:32.7.0(230.1*V)\r\n"
		"1-0:52.7.0(232.2*V)\r\n"
		"1-0:72.7.0(230.4*V)\r\n"
		"1-0:31.7.0(000.6*A)\r\n"
		"1-0:51.7.0(000.6*A)\r\n"
		"1-0:71.7.0(000.3*A)\r\n";

	struct parser *parser = parser_init();
	struct net_buf *buf = net_buf_alloc_with_data(&test_parser_buf_pool, test_data, sizeof(test_data)/sizeof(test_data[0]), K_NO_WAIT);
	zassert_not_null(buf);

	struct telegram *telegram = parse_telegram(parser, buf);
	zassert_not_null(telegram);
	zassert_equal(telegram_items_count(telegram), 8);

	net_buf_unref(buf);
	parser_free(parser);
}
