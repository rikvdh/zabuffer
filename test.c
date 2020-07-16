#include "zabuffer.h"

#include <stdint.h>
#include <string.h>
#include <assert.h>

static bool ret;

#define POOL_SIZE 4U
#define POOL_CHUNK_SIZE 16U

ZA_BUFFER_DECL(buf, 256)
ZA_BUFFER_LIST_DECL(pool, POOL_SIZE, POOL_CHUNK_SIZE);

static void test_expect_init(void)
{
	assert(0U == buf.used);
	assert(buf_data == buf.cur);
	assert(buf_data == buf.data);
	assert(sizeof(buf_data) == buf.size);
}

static void test_expect_destroy(void)
{
	assert(NULL == buf.cur);
	assert(NULL == buf.data);
	assert(0U ==   buf.size);
}

static void test_expect_element(size_t n)
{
	assert(n == buf.used);
	assert(&buf_data[n] == buf.cur);
	assert(buf_data == buf.data);
	assert(sizeof(buf_data) == buf.size);
}

void test_za_buffer_init(void) {
	za_buffer_init(&buf, buf_data, sizeof(buf_data));
	test_expect_init();
}

void test_za_buffer_destroy(void)
{
	za_buffer_destroy(&buf);
	test_expect_destroy();
}

void test_za_buffer_write_u8(void) {
	za_buffer_init(&buf, buf_data, sizeof(buf_data));

	za_buffer_write_u8(&buf, 'a');
	za_buffer_write_u8(&buf, 'b');
	za_buffer_write_u8(&buf, 'c');
	za_buffer_write_u8(&buf, 'd');
	za_buffer_write_u8(&buf, 0);

	assert(0 == strcmp("abcd", (const char *)buf.data));
}

void test_za_buffer_write_data(void) {
	za_buffer_init(&buf, buf_data, sizeof(buf_data));

	/* Write string with null termination */
	za_buffer_write_data(&buf, "1234", 5);
	assert(0 == strcmp("1234", (const char *)buf.data));
	assert(5U == buf.used);

	/* Set buffer to fake size of 3 bytes */
	za_buffer_init(&buf, buf_data, 3);

	/* Write more data then size of buffer */
	za_buffer_write_data(&buf, "1234", 5);

	/* Null terminate so we can check with STREQ */
	*buf.cur = 0;

	assert(0 == strcmp("12", (const char *)buf.data));
}

void test_za_buffer_next_reset(void) {
	za_buffer_init(&buf, buf_data, sizeof(buf_data));
	ret = za_buffer_next(&buf);

	assert(1 == ret);
	test_expect_element(1);

	za_buffer_reset(&buf);
	test_expect_init();
}

void test_za_buffer_next_until_end(void) {
	za_buffer_init(&buf, buf_data, sizeof(buf_data));

	for (size_t n = 0; n < sizeof(buf_data) - 1; n++)
		ret = za_buffer_next(&buf);

	assert(1 == ret);
	test_expect_element(255);
}

void test_za_buffer_next_overflow_check(void) {
	za_buffer_init(&buf, buf_data, sizeof(buf_data));

	for (size_t n = 0; n < sizeof(buf_data) * 2; n++)
		ret = za_buffer_next(&buf);

	assert(0 == ret);
	test_expect_element(255);
}

void test_za_buffer_next_pos_overflow_check(void) {
	za_buffer_init(&buf, buf_data, sizeof(buf_data));

	assert(0U ==       buf.used);
	assert(buf_data == buf.cur);

	buf.used = SIZE_MAX;
	ret = za_buffer_next(&buf);

	assert(0 == ret);
	assert(buf.used == SIZE_MAX);
	assert(buf_data == buf.cur);
}

void test_za_buffer_size_inuse_free(void) {
	const size_t in_use = 8U;
	za_buffer_init(&buf, buf_data, sizeof(buf_data));

	for (size_t n = 0; n < in_use; n++)
		ret = za_buffer_next(&buf);

	assert(1 == ret);
	assert(sizeof(buf_data) == (za_buffer_size_free(&buf) + za_buffer_size_inuse(&buf)));
	assert(in_use == za_buffer_size_inuse(&buf));
	assert((sizeof(buf_data) - in_use) == za_buffer_size_free(&buf));
}

void test_za_buffer_memcpy(void) {
	uint8_t data[16];
	uint8_t _expect[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };
	struct za_buffer b;
	size_t offset = 8;

	za_buffer_init(&b, data, sizeof(data));
	za_buffer_flush(&b);

	/* Write first 8 byte chunk with testdata */
	offset = 0;
	assert(sizeof(_expect) == za_buffer_memcpy(&b, offset, _expect, sizeof(_expect)));
	assert(0 == memcmp((uint8_t *)b.data + offset, _expect, sizeof(_expect)));
	za_buffer_flush(&b);

	/* Write first 8 byte chunk with testdata at offset 1 */
	offset = 1;
	assert(sizeof(_expect) == za_buffer_memcpy(&b, offset, _expect, sizeof(_expect)));
	assert(0 == memcmp((uint8_t *)b.data + offset, _expect, sizeof(_expect)));
	za_buffer_flush(&b);

	/* Write second 8 byte chunk with testdata at offset 7 (one byte before end of za_buffer) */
	offset = 7;
	assert(sizeof(_expect) == za_buffer_memcpy(&b, offset, _expect, sizeof(_expect)));
	assert(0 == memcmp((uint8_t *)b.data + offset, _expect, sizeof(_expect)));
	za_buffer_flush(&b);

	/* Wright second 8 byte chunk with testdata */
	offset = 8;
	assert(sizeof(_expect) == za_buffer_memcpy(&b, offset, _expect, sizeof(_expect)));
	assert(0 == memcmp((uint8_t *)b.data + offset, _expect, sizeof(_expect)));

	za_buffer_flush(&b);

	/* Try to memcpy one byte to much */
	offset = 9;
	assert(0U == za_buffer_memcpy(&b, offset, _expect, sizeof(_expect)));
}

void test_za_buffer_memmem(void) {
	const uint8_t buffer[] = "\x11\x12\x13\x14";
	const uint8_t search[] = "\x13";

	/* Match */
	const void *ptr = za_buffer_memmem((const void *)buffer, sizeof(buffer) - 1, (const void *)search, sizeof(search) - 1);
	assert(&buffer[2] == ptr);

	/* 0-length for l_len */
	const void *ptr2 = za_buffer_memmem((const void *)buffer, 0, (const void *)search, sizeof(search) - 1);
	assert(NULL == ptr2);

	/* 0-length for s_len */
	const void *ptr3 = za_buffer_memmem((const void *)buffer, sizeof(buffer) - 1, (const void *)search, 0);
	assert(NULL == ptr3);

	/* Check length boundary isn't exceeded */
	const uint8_t alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	const uint8_t searchbuf[] = "U";

	const void *ptr4 = za_buffer_memmem((const void *)alphabet, 10, (const void *)searchbuf, 1);
	assert(NULL == ptr4);
}

void test_za_buffer_list_init(void) {
	ZA_BUFFER_LIST_INIT(pool, POOL_SIZE, POOL_CHUNK_SIZE);

	// Test if the pool buffer pointer is correctly set
	assert(pool.buffer == pool_buffer);

	for (size_t n = 0; n < POOL_SIZE; n++) {
		assert(0 == pool.buffer[n].in_use);
		assert(0U == pool.buffer[n].used);
		assert(POOL_CHUNK_SIZE == pool.buffer[n].size);
		assert(pool_data[n] == pool.buffer[n].data);
		assert(pool_data[n] == pool.buffer[n].cur);
	}
}

void test_za_buffer_list_get_return(void) {
	for (size_t n = 0; n < POOL_SIZE; n++) {
		assert(0 == pool.buffer[n].in_use);
		assert(&pool.buffer[n] == za_buffer_list_get(&pool));
		assert(1 == pool.buffer[n].in_use);
	}

	for (size_t n = 0; n < POOL_SIZE; n++) {
		struct za_buffer *b = &pool.buffer[n];
		za_buffer_list_return(&pool, &b);
		assert(0 == pool.buffer[n].in_use);
	}
}

void test_za_buffer_list_get_null(void) {
	for (size_t n = 0; n < POOL_SIZE; n++) {
		assert(0 == pool.buffer[n].in_use);
		assert(&pool.buffer[n] == za_buffer_list_get(&pool));
		assert(1 == pool.buffer[n].in_use);
	}

	assert(NULL == za_buffer_list_get(&pool));
}

void test_za_buffer_list_return_invalid(void) {
	za_buffer_list_return(NULL, (struct za_buffer **)NULL);
	za_buffer_list_return(&pool, (struct za_buffer **)NULL);
}

/** Check if buf->data is not flushed on reset
 */
void test_za_buffer_reset(void) {
	const char *text = "Hello World!";
	const size_t size = strlen(text) + 1;

	za_buffer_init(&buf, buf_data, sizeof(buf_data));
	assert(size == za_buffer_memcpy(&buf, 0, text, size));
	assert(0 == strcmp(text, (const char *)buf.data));

	za_buffer_reset(&buf);
	assert(0 == strcmp(text, (const char *)buf.data));
	test_expect_init();
}

/** Check if reset sets the pos, used and cur members */
void test_za_buffer_reset_members(void) {
	za_buffer_init(&buf, buf_data, sizeof(buf_data));

	buf.used = 1234;
	buf.cur  = (uint8_t *)&buf_data + 10;

	za_buffer_reset(&buf);

	assert(0U == buf.used);
	assert(buf_data == buf.cur);
}

int main(void)
{
    test_za_buffer_init();
    test_za_buffer_destroy();
    test_za_buffer_write_u8();
    test_za_buffer_write_data();
    test_za_buffer_next_reset();
    test_za_buffer_next_until_end();
    test_za_buffer_next_overflow_check();
    test_za_buffer_next_pos_overflow_check();
    test_za_buffer_size_inuse_free();
    test_za_buffer_memcpy();
    test_za_buffer_memmem();
    test_za_buffer_list_init();
    test_za_buffer_list_get_return();
    test_za_buffer_list_get_null();
    test_za_buffer_list_return_invalid();
    test_za_buffer_reset();
    test_za_buffer_reset_members();
    return 0;
}
