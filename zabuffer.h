#ifndef ZA_BUFFER_H__
#define ZA_BUFFER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define ZA_BUFFER_DECL_ARRAY(name, elements, size) \
	static uint8_t name##_data[elements][size]; \
	static struct za_buffer name[elements];

#define ZA_BUFFER_INIT_ARRAY(name, elements, size) \
	za_buffer_init_array(name, name##_data, elements, size)

#define ZA_BUFFER_DECL(name, size) \
	static uint8_t name##_data[size]; \
	static struct za_buffer name = { name##_data, name##_data, size, 0, false };

/**
 * Create buffers with list
 * - data:   NAME_data[LIST_SIZE][DATA_SIZE]
 * - buffer: NAME_buffer
 * - list:   NAME
 */
#define ZA_BUFFER_LIST_DECL(name, list_size, data_size) \
	static uint8_t name##_data[list_size][data_size]; \
	static struct  za_buffer name##_buffer[list_size]; \
	static struct  za_buffer_list name

#define ZA_BUFFER_LIST_INIT(name, list_size, data_size) \
	za_buffer_list_init(&name, list_size, name##_buffer, &name##_data, data_size)

/**
 * Buffer
 */
struct za_buffer {
	uint8_t *data;
	uint8_t *cur;
	size_t size;
	size_t used;
	bool in_use;
};

/**
 * List of za_buffer with locking
 */
struct za_buffer_list {
	struct za_buffer *buffer;
	size_t size;
};

/**
 * Initialize buffer, it will not flush the buffer. Please use za_buffer_flush.
 */
void za_buffer_init(struct za_buffer *b, void *data, size_t size);

void za_buffer_init_array(struct za_buffer *a, void *data, size_t elements, size_t size);

void za_buffer_destroy(struct za_buffer *b);

bool za_buffer_is_full(const struct za_buffer *b);

/**
 * Reset buffer to begin
 * * Set cur member
 * * Reset used member
 */
void za_buffer_reset(struct za_buffer *b);

/**
 * Flush buffer data to zero and reset
 */
void za_buffer_flush(struct za_buffer *b);

/**
 * Append byte to buffer
 */
bool za_buffer_write_u8(struct za_buffer *b, uint8_t c);

/**
 * Append buffer to buffer
 */
size_t za_buffer_write_data(struct za_buffer *b, const void *buf, size_t len);

/**
 * Set cursor to next byte
 */
bool za_buffer_next(struct za_buffer *b);

/**
 * Get size in used bytes
 */
size_t za_buffer_size_inuse(const struct za_buffer *b);

/**
 * Get amount of free bytes
 */
size_t za_buffer_size_free(const struct za_buffer *b);

/**
 * Memory copy buf with n size into b->data + offset, when
 *  it won't fit then no bytes are written because memcpy truncating
 *  bytes is not normal behavior
 * @return Size of written bytes (0 if it exceeds b->data + b->size)
 */
size_t za_buffer_memcpy(const struct za_buffer *b, size_t offset, const void *buf, size_t n);

/**
 * Search for pattern in memory and return at occurrence
 * @param l Memory block
 * @param l_len Memory block length
 * @param s Pattern to search for
 * @param s_len Pattern length
 */
void *za_buffer_memmem(const void *l, size_t l_len, const void *s, size_t s_len);

/**
 * Append buffer to another
 * Deep copies src->used bytes starting from offset 0 into dst starting from current offset
 *  the data must fit else no copy will occur
 * @param dst Destination buffer
 * @param src Source buffer
 */
int za_buffer_append(struct za_buffer *dst, struct za_buffer *src);

/**
 * Initialize list of buffers<br>
 * Example:
 * - LIST_SIZE: 10
 * - OBJ_SIZE: 256
 * - l: za_buffer list[LIST_SIZE]
 * - l_n: LIST_SIZE
 * - data: uint8_t list_data[LIST_SIZE][OBJ_SIZE]
 * - data_n: OBJ_SIZE
 * @param list   List of buffer objects
 * @param list_n Amount of items in list
 * @param buffer Buffer
 * @param data   Data block, MUST have a size of l_n * data_n
 * @param data_n Data block object size
 */
void za_buffer_list_init(struct za_buffer_list *list, size_t list_n, struct za_buffer *buffer,
			 void *data, size_t data_n);

struct za_buffer *za_buffer_list_get(struct za_buffer_list *list);
void za_buffer_list_return(struct za_buffer_list *list, struct za_buffer **b);

#ifdef __cplusplus
}
#endif

#endif /** ZA_BUFFER_H__ */
