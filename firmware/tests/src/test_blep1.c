#include <regex.h>

#include <zephyr/ztest.h>

#include "lib/openp1.h"

ZTEST_SUITE(openp1_suite, NULL, NULL, NULL, NULL, NULL);

ZTEST(openp1_suite, test_definitions_for_all_items) {
	for (int i = 0 ; i < _ITEM_COUNT ; i++) {
		zassert_equal(data_definition_table[i].item, i);
	}
}

ZTEST(openp1_suite, test_ble_definitions_for_all_items) {
	for (int i = 0 ; i < _ITEM_COUNT ; i++) {
		zassert_equal(gatt_ch_table[i].item, i);
	}
}
