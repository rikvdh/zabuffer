#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "zabuffer.h"

void za_buffer_init(struct za_buffer *b, void *data, size_t size)
{
	b->in_use = false;
	b->used   = 0;
	b->size   = size;
	b->data   = data;
	b->cur    = data;
}

void za_buffer_init_array(struct za_buffer *list, void *data, size_t elements, size_t size)
{
	void *_data;
	struct za_buffer *b;

	for (size_t n = 0; n < elements; n++) {
		b = &list[n];

		_data = (uint8_t *)data + (n * size);
		za_buffer_init(b, _data, size);
	}
}

void za_buffer_destroy(struct za_buffer *b)
{
	memset(b, 0, sizeof(*b));
}

void za_buffer_reset(struct za_buffer *b)
{
	b->used = 0;
	b->cur  = b->data;
}

void za_buffer_flush(struct za_buffer *b)
{
	if (b->data == NULL) {
		return;
	}

	memset(b->data, 0, b->size);
	za_buffer_reset(b);
}

bool za_buffer_next(struct za_buffer *b)
{
	if (b->data == NULL) {
		return false;
	}
	if (b->used == SIZE_MAX) {
		return false;
	}
	if (za_buffer_is_full(b)) {
		return false;
	}

	b->used++;
	b->cur++;

	return true;
}

bool za_buffer_is_full(const struct za_buffer *b)
{
	if (b->used < (b->size - 1)) {
		return false;
	}
	return true;
}

size_t za_buffer_size_inuse(const struct za_buffer *b)
{
	return b->used;
}

size_t za_buffer_size_free(const struct za_buffer *b)
{
	return b->size - b->used;
}

bool za_buffer_write_u8(struct za_buffer *b, uint8_t c)
{
	if (!za_buffer_next(b)) {
		return false;
	}

	*(b->cur - 1) = c;

	return true;
}

size_t za_buffer_write_data(struct za_buffer *b, const void *buf, size_t len)
{
	size_t n;

	for (n = 0; n < len; n++) {
		if (!za_buffer_write_u8(b, ((uint8_t *)buf)[n])) {
			break;
		}
	}

	return n;
}

size_t za_buffer_memcpy(const struct za_buffer *b, size_t offset, const void *buf, size_t n)
{
	if ((offset + n) > b->size) {
		return 0;
	}

	memcpy((uint8_t *)(b->data) + offset, buf, n);

	return n;
}

void *za_buffer_memmem(const void *l, size_t l_len, const void *s, size_t s_len)
{
	register uint8_t *cur, *last;
	const uint8_t *cl = (const uint8_t *)l;
	const uint8_t *cs = (const uint8_t *)s;

	/* we need something to compare */
	if (l_len == 0 || s_len == 0) {
		return NULL;
	}

	/* "s" must be smaller or equal to "l" */
	if (l_len < s_len) {
		return NULL;
	}

	/* special case where s_len == 1 */
	if (s_len == 1) {
		return memchr(l, (int)*cs, l_len);
	}

	/* the last position where its possible to find "s" in "l" */
	last = (uint8_t *)cl + l_len - s_len;

	for (cur = (uint8_t *)cl; cur <= last; cur++) {
		if (cur[0] == cs[0] && memcmp(cur, cs, s_len) == 0) {
			return cur;
		}
	}

	return NULL;
}

int za_buffer_append(struct za_buffer *dst, struct za_buffer *src)
{
	size_t n;

	if (src->used == 0) {
		return 0;
	}

	if (za_buffer_size_free(dst) < src->used) {
		return -1;
	}

	n = za_buffer_memcpy(dst, dst->used, src->data, src->used);
	if (n == 0) {
		return -1;
	}
	dst->used += src->used;

	return 0;
}

void za_buffer_list_init(struct za_buffer_list *list, size_t list_n, struct za_buffer *buffer,
			 void *data, size_t data_n)
{
	list->buffer = buffer;
	list->size   = list_n;

	for (size_t n = 0; n < list_n; n++) {
		za_buffer_init(&list->buffer[n], (uint8_t *)(data) + (n * data_n), data_n);
	}
}

struct za_buffer *za_buffer_list_get(struct za_buffer_list *list)
{
	struct za_buffer *b = NULL;

	for (size_t n = 0; n < list->size; n++) {
		if (!list->buffer[n].in_use) {
			b = &list->buffer[n];
			b->in_use = true;
			za_buffer_reset(b);
			break;
		}
	}

	return b;
}

void za_buffer_list_return(struct za_buffer_list *list, struct za_buffer **b)
{
	if (list == NULL || b == NULL) {
		return;
	}

	for (size_t n = 0; n < list->size; n++) {
		if (&list->buffer[n] == *b) {
			(*b)->in_use = false;
			*b = NULL;
			break;
		}
	}
}
