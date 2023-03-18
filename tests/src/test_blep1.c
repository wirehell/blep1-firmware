#include <regex.h>
#include "lib/blep1.h"

#include <zephyr/ztest.h>

ZTEST_SUITE(blep1_suite, NULL, NULL, NULL, NULL, NULL);

ZTEST(blep1_suite, test_definitions_for_all_values) {
	for (int i = 0 ; i < _ITEM_COUNT ; i++) {
		zassert_equal(data_definition_table[i].item, i);
	}
}

