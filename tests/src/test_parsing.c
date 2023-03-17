#include <regex.h>
#include "telegram_framer.h"
#include "parser.h"
#include "common.h"

#include <zephyr/ztest.h>


// Test framing + parsing

ZTEST_SUITE(telegram_parsing_suite, NULL, NULL, NULL, NULL, NULL);

ZTEST(telegram_parsing_suite, test_landis_gyr_e360) {
	uint8_t *test_data[] = {
		"/LGF5E360\r\n\r\n",
		"0-0:1.0.0(210222161900W)\r\n", 
		"1-0:1.8.0(00000896.020*kWh)\r\n",
		"1-0:2.8.0(00000048.792*kWh)\r\n",
		"1-0:3.8.0(00000518.309*kVArh)\r\n",
		"1-0:4.8.0(00000023.732*kVArh)\r\n",
		"1-0:1.7.0(0000.000*kW)\r\n",
		"1-0:2.7.0(0000.020*kW)\r\n",
		"1-0:3.7.0(0000.000*kVAr)\r\n",
		"1-0:4.7.0(0000.308*kVAr)\r\n",
		"1-0:21.7.0(0000.000*kW)\r\n",
		"1-0:22.7.0(0000.012*kW)\r\n",
		"1-0:41.7.0(0000.000*kW)\r\n",
		"1-0:42.7.0(0000.071*kW)\r\n",
		"1-0:61.7.0(0000.063*kW)\r\n",
		"1-0:62.7.0(0000.000*kW)\r\n",
		"1-0:23.7.0(0000.000*kVAr)\r\n",
		"1-0:24.7.0(0000.146*kVAr)\r\n",
		"1-0:43.7.0(0000.000*kVAr)\r\n",
		"1-0:44.7.0(0000.135*kVAr)\r\n",
		"1-0:63.7.0(0000.000*kVAr)\r\n",
		"1-0:64.7.0(0000.026*kVAr)\r\n",
		"1-0:32.7.0(230.1*V)\r\n",
		"1-0:52.7.0(232.2*V)\r\n",
		"1-0:72.7.0(230.4*V)\r\n",
		"1-0:31.7.0(000.6*A)\r\n",
		"1-0:51.7.0(000.6*A)\r\n",
		"1-0:71.7.0(000.3*A)\r\n",
		"!A077\r"
	};
	struct telegram_framer *framer = telegram_framer_init();
	zassert_not_null(framer);
	for (int i = 0 ; i < sizeof(test_data) / sizeof(*test_data) ; i++) {
		uint8_t *s = test_data[i];
		for (int j = 0 ; j < strlen(s) ; j++) {
			zassert_is_null(telegram_framer_push(framer, s[j]));
		}
	}
	struct net_buf *buf = telegram_framer_push(framer, '\n');
	zassert_not_null(buf);

	struct parser *parser = parser_init();

	struct telegram *telegram = parse_telegram(parser, buf);
	zassert_not_null(telegram);

	// TODO verify properties
	printf("Size of telegram: %d", telegram_items_count(telegram));

	parser_free(parser);
	telegram_framer_free(framer);
}
