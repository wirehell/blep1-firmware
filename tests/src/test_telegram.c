#include <regex.h>
#include "telegram.h"
#include "common.h"

#include <zephyr/ztest.h>

ZTEST_SUITE(telegram_suite, NULL, NULL, NULL, NULL, NULL);

ZTEST(telegram_suite, test_empty)
{
	struct telegram *telegram = telegram_init();
	zassert_not_null(telegram);

	struct telegram_data_iterator iter;
	telegram_item_iterator_init(telegram, &iter);

	zassert_equal(telegram_items_count(telegram), 0);
	zassert_is_null(telegram_item_iterator_next(&iter));

	telegram_free(telegram);
}

ZTEST(telegram_suite, test_multiple)
{
	struct data_item items[] = {
		{ METER_ACTIVE_ENERGY_IN, { .double_long_unsigned = 321000 }},
		{ METER_ACTIVE_ENERGY_OUT, { .long_unsigned = 1000 }},
	};

	struct telegram *telegram = telegram_init();
	zassert_not_null(telegram);
	int item_count = sizeof(items)/sizeof(items[0]);

	for (int i = 0 ; i < item_count ; i++) {
		telegram_item_append(telegram, &items[i]);
		zassert_equal(telegram_items_count(telegram), i + 1);
	}

	struct telegram_data_iterator iter;
	telegram_item_iterator_init(telegram, &iter);
	int j = 0;
	struct data_item *item;
	while (NULL != (item = telegram_item_iterator_next(&iter))) {
		zassert_equal(item->item, items[j].item);
		zassert_equal(item->value.double_long_unsigned, items[j].value.double_long_unsigned);
		j++;
	}
	zassert_equal(j, item_count);

	telegram_free(telegram);
}