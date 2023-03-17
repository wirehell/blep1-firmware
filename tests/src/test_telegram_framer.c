#include <regex.h>
#include "telegram_framer.h"
#include "common.h"

#include <zephyr/ztest.h>

ZTEST_SUITE(telegram_framer_suite, NULL, NULL, NULL, NULL, NULL);

ZTEST(telegram_framer_suite, test_simple_frame)
{
	uint8_t *test_data[] = {
		"/ASD5id123\r\n\r\n",
		"1-0:1.8.0(00006678.394*kWh)\r\n",
		"1-0:2.8.0(00000000.000*kWh)\r\n",
		"1-0:2.9.0(00000000.000)\r\n",
		"1-0:21.7.0(0001.023*kW)\r\n",
		"!10bc\r" // "\n" missing.
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
	net_buf_unref(buf);
	telegram_framer_free(framer);

}

ZTEST(telegram_framer_suite, test_preceeding_garbage) 
{
	uint8_t *test_data[] = {
		"asdf\r\n",
		"fs/ASD5id123\r\n\r\n",
		"1-0:1.8.0(00006678.394*kWh)\r\n",
		"1-0:2.8.0(00000000.000*kWh)\r\n",
		"1-0:2.9.0(00000000.000)\r\n",
		"1-0:21.7.0(0001.023*kW)\r\n",
		"!10bc\r\n",
	};

	struct telegram_framer *framer = telegram_framer_init();
	zassert_not_null(framer);
	struct net_buf *buf = NULL;
	for (int i = 0 ; i < sizeof(test_data) / sizeof(*test_data) ; i++) {
		uint8_t *s = test_data[i];
		for (int j = 0 ; j < strlen(s) ; j++) {
			buf = telegram_framer_push(framer, s[j]);
		}
	}
	zassert_not_null(buf);
	net_buf_unref(buf);
	telegram_framer_free(framer);

}

ZTEST(telegram_framer_suite, test_many_frames) 
{
	uint8_t *test_data[] = {
		"/ASD5id123\r\n\r\n",
		"1-0:1.8.0(00006678.394*kWh)\r\n",
		"1-0:2.8.0(00000000.000*kWh)\r\n",
		"1-0:2.9.0(00000000.000)\r\n",
		"1-0:21.7.0(0001.023*kW)\r\n",
		"!10bc\r",
	};

	struct telegram_framer *framer = telegram_framer_init();
	zassert_not_null(framer);

	for (int k = 0 ; k < 100 ; k++) {
		for (int i = 0 ; i < sizeof(test_data) / sizeof(*test_data) ; i++) {
			uint8_t *s = test_data[i];
			for (int j = 0 ; j < strlen(s) ; j++) {
				zassert_is_null(telegram_framer_push(framer, s[j]));
			}
		}
		struct net_buf *buf = telegram_framer_push(framer, '\n');
		zassert_not_null(buf, "Missing buf on iteration: %d", k);
		net_buf_unref(buf);
	}
	telegram_framer_free(framer);
}

ZTEST(telegram_framer_suite, test_overflow_and_recovery) 
{
	uint8_t *test_data[] = {
		"/ASD5id123\r\n\r\n",
		"1-0:1.8.0(00006678.394*kWh)\r\n",
		"1-0:2.8.0(00000000.000*kWh)\r\n",
		"1-0:2.9.0(00000000.000)\r\n",
		"1-0:21.7.0(0001.023*kW)\r\n",
		"!10bc\r" // "\n" missing.
	};

	struct telegram_framer *framer = telegram_framer_init();
	zassert_not_null(framer);

	for (int i = 0 ; i < 50000 ; i++) {
		telegram_framer_push(framer, 'a');
	}

	for (int i = 0 ; i < sizeof(test_data) / sizeof(*test_data) ; i++) {
		uint8_t *s = test_data[i];
		for (int j = 0 ; j < strlen(s) ; j++) {
			zassert_is_null(telegram_framer_push(framer, s[j]));
		}
	}
	struct net_buf *buf = telegram_framer_push(framer, '\n');
	zassert_not_null(buf);
	net_buf_unref(buf);
	telegram_framer_free(framer);
}

ZTEST(telegram_framer_suite, test_garbage_reset_and_recovery) 
{
	uint8_t *test_data[] = {
		"/ASD5id123\r\n\r\n",
		"1-0:1.8.0(00006678.394*kWh)\r\n",
		"1-0:2.8.0(00000000.000*kWh)\r\n",
		"1-0:2.9.0(00000000.000)\r\n",
		"1-0:21.7.0(0001.023*kW)\r\n",
		"!10bc\r" // "\n" missing.
	};

	struct telegram_framer *framer = telegram_framer_init();
	zassert_not_null(framer);

	for (int i = 0 ; i < 50000 ; i++) {
		telegram_framer_push(framer, (char) i);
	}
	telegram_framer_reset(framer);

	for (int i = 0 ; i < sizeof(test_data) / sizeof(*test_data) ; i++) {
		uint8_t *s = test_data[i];
		for (int j = 0 ; j < strlen(s) ; j++) {
			zassert_is_null(telegram_framer_push(framer, s[j]));
		}
	}
	struct net_buf *buf = telegram_framer_push(framer, '\n');
	zassert_not_null(buf);
	net_buf_unref(buf);
	telegram_framer_free(framer);
}

ZTEST(telegram_framer_suite, test_bad_crc)
{
	uint8_t *test_data[] = {
		"/ASD5id123\r\n\r\n",
		"1-0:1.8.0(00006678.394*kWh)\r\n",
		"1-0:2.8.0(00000000.000*kWh)\r\n",
		"1-0:2.9.0(00000000.000)\r\n",
		"1-0:21.7.0(0001.023*kW)\r\n",
		"!ABBA\r\n" 
	};

	struct telegram_framer *framer = telegram_framer_init();
	zassert_not_null(framer);
	for (int i = 0 ; i < sizeof(test_data) / sizeof(*test_data) ; i++) {
		uint8_t *s = test_data[i];
		for (int j = 0 ; j < strlen(s) ; j++) {
			zassert_is_null(telegram_framer_push(framer, s[j]));
		}
	}
	telegram_framer_free(framer);
}

ZTEST(telegram_framer_suite, test_landis_gyr_e360) {
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
	net_buf_unref(buf);
	telegram_framer_free(framer);
}
