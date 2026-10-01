#include "NbtSerializer.h"
#include "NbtTags.h"
#include "Zend/zend_smart_str.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#ifdef BIG_ENDIAN
#undef BIG_ENDIAN
#endif
#ifdef LITTLE_ENDIAN
#undef LITTLE_ENDIAN
#endif

zend_class_entry *base_nbt_serializer_ce = NULL;
zend_class_entry *big_endian_nbt_serializer_ce = NULL;
zend_class_entry *little_endian_nbt_serializer_ce = NULL;
zend_class_entry *network_nbt_serializer_ce = NULL;

// C implementations of the primitive buffer readers and writers.
typedef enum {
	TYPE_BIG_ENDIAN,
	TYPE_LITTLE_ENDIAN,
	TYPE_NETWORK
} NbtSerializerType;
typedef struct {
	const unsigned char *data;
	size_t size;
	size_t *offset;
	NbtSerializerType type;
} FastBufferReader;
typedef struct {
	smart_str buffer;
	NbtSerializerType type;
} FastBufferWriter;

static uint16_t nbt_bswap16(uint16_t v) {
	return (uint16_t)((v << 8) | (v >> 8));
}
static uint32_t nbt_bswap32(uint32_t v) {
	return ((v >> 24) & 255) | ((v >> 8) & 0xff00) | ((v << 8) & 0xff0000) |
		   (v << 24);
}
static uint64_t nbt_bswap64(uint64_t v) {
	return ((v & 0xffULL) << 56) | ((v & 0xff00ULL) << 40) |
		   ((v & 0xff0000ULL) << 24) | ((v & 0xff000000ULL) << 8) |
		   ((v >> 8) & 0xff000000ULL) | ((v >> 24) & 0xff0000ULL) |
		   ((v >> 40) & 0xff00ULL) | ((v >> 56) & 0xffULL);
}
static zend_bool nbt_is_little_endian(void) {
	const uint16_t v = 1;
	return *((const uint8_t *)&v) == 1;
}
static zend_bool nbt_needs_swap(NbtSerializerType type) {
	return (type == TYPE_BIG_ENDIAN && nbt_is_little_endian()) ||
		   (type == TYPE_LITTLE_ENDIAN && !nbt_is_little_endian());
}
static NbtSerializerType get_serializer_type(zval *obj) {
	if (network_nbt_serializer_ce &&
		instanceof_function(Z_OBJCE_P(obj), network_nbt_serializer_ce))
		return TYPE_NETWORK;
	if (big_endian_nbt_serializer_ce &&
		instanceof_function(Z_OBJCE_P(obj), big_endian_nbt_serializer_ce))
		return TYPE_BIG_ENDIAN;
	return TYPE_LITTLE_ENDIAN;
}
static zend_bool reader_has_bytes(FastBufferReader *r, size_t n) {
	return *r->offset <= r->size && n <= r->size - *r->offset;
}
static zend_bool reader_require(FastBufferReader *r, size_t n) {
	if (reader_has_bytes(r, n))
		return 1;
	throw_nbt_data_exception(
		"Unexpected end of stream: expected %zu bytes, but only %zu available "
		"at offset %zu",
		n, *r->offset <= r->size ? r->size - *r->offset : 0, *r->offset);
	return 0;
}
static uint8_t reader_byte(FastBufferReader *r) {
	if (!reader_require(r, 1))
		return 0;
	return r->data[(*r->offset)++];
}
static uint16_t reader_short(FastBufferReader *r) {
	uint16_t v;
	if (!reader_require(r, 2))
		return 0;
	memcpy(&v, r->data + *r->offset, 2);
	*r->offset += 2;
	return nbt_needs_swap(r->type) ? nbt_bswap16(v) : v;
}
static uint32_t reader_varuint(FastBufferReader *r) {
	uint32_t v = 0;
	int i;
	for (i = 0; i < 35; i += 7) {
		uint8_t b;
		if (!reader_require(r, 1))
			return 0;
		b = r->data[(*r->offset)++];
		v |= (uint32_t)(b & 127) << i;
		if (!(b & 128))
			return v;
	}
	throw_nbt_data_exception("VarInt did not terminate after 5 bytes!");
	return 0;
}
static int32_t reader_varint(FastBufferReader *r) {
	uint32_t v = reader_varuint(r);
	return (int32_t)((v >> 1) ^ -(int32_t)(v & 1));
}
static uint64_t reader_varulong(FastBufferReader *r) {
	uint64_t v = 0;
	int i;
	for (i = 0; i < 70; i += 7) {
		uint8_t b;
		if (!reader_require(r, 1))
			return 0;
		b = r->data[(*r->offset)++];
		v |= (uint64_t)(b & 127) << i;
		if (!(b & 128))
			return v;
	}
	throw_nbt_data_exception("VarLong did not terminate after 10 bytes!");
	return 0;
}
static int64_t reader_varlong(FastBufferReader *r) {
	uint64_t v = reader_varulong(r);
	return (int64_t)((v >> 1) ^ -(int64_t)(v & 1));
}
static uint32_t reader_int(FastBufferReader *r) {
	uint32_t v;
	if (r->type == TYPE_NETWORK)
		return (uint32_t)reader_varint(r);
	if (!reader_require(r, 4))
		return 0;
	memcpy(&v, r->data + *r->offset, 4);
	*r->offset += 4;
	return nbt_needs_swap(r->type) ? nbt_bswap32(v) : v;
}
static uint64_t reader_long(FastBufferReader *r) {
	uint64_t v;
	if (r->type == TYPE_NETWORK)
		return (uint64_t)reader_varlong(r);
	if (!reader_require(r, 8))
		return 0;
	memcpy(&v, r->data + *r->offset, 8);
	*r->offset += 8;
	return nbt_needs_swap(r->type) ? nbt_bswap64(v) : v;
}
static float reader_float(FastBufferReader *r) {
	uint32_t v;
	float f;
	if (!reader_require(r, 4))
		return 0;
	memcpy(&v, r->data + *r->offset, 4);
	*r->offset += 4;
	if (nbt_needs_swap(r->type))
		v = nbt_bswap32(v);
	memcpy(&f, &v, 4);
	return f;
}
static double reader_double(FastBufferReader *r) {
	uint64_t v;
	double d;
	if (!reader_require(r, 8))
		return 0;
	memcpy(&v, r->data + *r->offset, 8);
	*r->offset += 8;
	if (nbt_needs_swap(r->type))
		v = nbt_bswap64(v);
	memcpy(&d, &v, 8);
	return d;
}
static zend_string *reader_string(FastBufferReader *r) {
	uint32_t n = r->type == TYPE_NETWORK ? reader_varuint(r) : reader_short(r);
	zend_string *out;
	if (EG(exception))
		return zend_string_init("", 0, 0);
	if (n > 32767) {
		throw_nbt_data_exception("NBT string length too large (%u > 32767)", n);
		return zend_string_init("", 0, 0);
	}
	if (!reader_require(r, n))
		return zend_string_init("", 0, 0);
	out = zend_string_init((const char *)r->data + *r->offset, n, 0);
	*r->offset += n;
	return out;
}
static zend_string *reader_bytes(FastBufferReader *r) {
	int32_t n = (int32_t)reader_int(r);
	zend_string *out;
	if (EG(exception))
		return zend_string_init("", 0, 0);
	if (n < 0) {
		throw_nbt_data_exception(
			"Array length cannot be less than zero (%d < 0)", n);
		return zend_string_init("", 0, 0);
	}
	if (!reader_require(r, (size_t)n))
		return zend_string_init("", 0, 0);
	out = zend_string_init((const char *)r->data + *r->offset, (size_t)n, 0);
	*r->offset += (size_t)n;
	return out;
}
static void reader_int_array(FastBufferReader *r, zval *a) {
	int32_t n = (int32_t)reader_int(r), i;
	if (EG(exception))
		return;
	if (n < 0) {
		throw_nbt_data_exception(
			"Array length cannot be less than zero (%d < 0)", n);
		return;
	}
	array_init_size(a, (uint32_t)n);
	for (i = 0; i < n; i++) {
		add_next_index_long(a, (int32_t)reader_int(r));
		if (EG(exception)) {
			zval_ptr_dtor(a);
			ZVAL_UNDEF(a);
			return;
		}
	}
}
static void writer_append(FastBufferWriter *w, const char *p, size_t n) {
	if (n)
		smart_str_appendl(&w->buffer, p, n);
}
static void writer_byte(FastBufferWriter *w, uint8_t v) {
	smart_str_appendc(&w->buffer, (char)v);
}
static void writer_short(FastBufferWriter *w, uint16_t v) {
	uint16_t x = nbt_needs_swap(w->type) ? nbt_bswap16(v) : v;
	writer_append(w, (char *)&x, 2);
}
static void writer_varuint(FastBufferWriter *w, uint32_t v) {
	int i;
	for (i = 0; i < 5; i++) {
		uint8_t b = v & 127;
		v >>= 7;
		if (v)
			writer_byte(w, b | 128);
		else {
			writer_byte(w, b);
			return;
		}
	}
	zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0,
							"Value too large to be encoded as a VarInt");
}
static void writer_varint(FastBufferWriter *w, int32_t v) {
	writer_varuint(w, ((uint32_t)v << 1) ^ (uint32_t)(v >> 31));
}
static void writer_varulong(FastBufferWriter *w, uint64_t v) {
	int i;
	for (i = 0; i < 10; i++) {
		uint8_t b = v & 127;
		v >>= 7;
		if (v)
			writer_byte(w, b | 128);
		else {
			writer_byte(w, b);
			return;
		}
	}
	zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0,
							"Value too large to be encoded as a VarLong");
}
static void writer_varlong(FastBufferWriter *w, int64_t v) {
	writer_varulong(w, ((uint64_t)v << 1) ^ (uint64_t)(v >> 63));
}
static void writer_int(FastBufferWriter *w, uint32_t v) {
	uint32_t x;
	if (w->type == TYPE_NETWORK) {
		writer_varint(w, (int32_t)v);
		return;
	}
	x = nbt_needs_swap(w->type) ? nbt_bswap32(v) : v;
	writer_append(w, (char *)&x, 4);
}
static void writer_long(FastBufferWriter *w, uint64_t v) {
	uint64_t x;
	if (w->type == TYPE_NETWORK) {
		writer_varlong(w, (int64_t)v);
		return;
	}
	x = nbt_needs_swap(w->type) ? nbt_bswap64(v) : v;
	writer_append(w, (char *)&x, 8);
}
static void writer_float(FastBufferWriter *w, float f) {
	uint32_t v;
	memcpy(&v, &f, 4);
	if (nbt_needs_swap(w->type))
		v = nbt_bswap32(v);
	writer_append(w, (char *)&v, 4);
}
static void writer_double(FastBufferWriter *w, double d) {
	uint64_t v;
	memcpy(&v, &d, 8);
	if (nbt_needs_swap(w->type))
		v = nbt_bswap64(v);
	writer_append(w, (char *)&v, 8);
}
static void writer_string(FastBufferWriter *w, const char *s, size_t n) {
	if (n > 32767) {
		zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0,
								"NBT string length too large (%zu > 32767)", n);
		return;
	}
	if (w->type == TYPE_NETWORK)
		writer_varuint(w, (uint32_t)n);
	else
		writer_short(w, (uint16_t)n);
	writer_append(w, s, n);
}
static void writer_bytes(FastBufferWriter *w, const char *s, size_t n) {
	writer_int(w, (uint32_t)n);
	writer_append(w, s, n);
}
static void decode_tag(FastBufferReader *r, int type, int max_depth, int depth,
					   zval *out) {
	switch (type) {
	case NBT_TAG_BYTE:
		object_init_ex(out, byte_tag_ce);
		zend_update_property_long(byte_tag_ce, Z_OBJ_P(out), "value", 5,
								  (int8_t)reader_byte(r));
		break;
	case NBT_TAG_SHORT:
		object_init_ex(out, short_tag_ce);
		zend_update_property_long(short_tag_ce, Z_OBJ_P(out), "value", 5,
								  (int16_t)reader_short(r));
		break;
	case NBT_TAG_INT:
		object_init_ex(out, int_tag_ce);
		zend_update_property_long(int_tag_ce, Z_OBJ_P(out), "value", 5,
								  (int32_t)reader_int(r));
		break;
	case NBT_TAG_LONG:
		object_init_ex(out, long_tag_ce);
		zend_update_property_long(long_tag_ce, Z_OBJ_P(out), "value", 5,
								  (zend_long)reader_long(r));
		break;
	case NBT_TAG_FLOAT:
		object_init_ex(out, float_tag_ce);
		zend_update_property_double(float_tag_ce, Z_OBJ_P(out), "value", 5,
									reader_float(r));
		break;
	case NBT_TAG_DOUBLE:
		object_init_ex(out, double_tag_ce);
		zend_update_property_double(double_tag_ce, Z_OBJ_P(out), "value", 5,
									reader_double(r));
		break;
	case NBT_TAG_BYTE_ARRAY:
	case NBT_TAG_STRING: {
		zend_string *s =
			type == NBT_TAG_BYTE_ARRAY ? reader_bytes(r) : reader_string(r);
		zend_class_entry *ce =
			type == NBT_TAG_BYTE_ARRAY ? byte_array_tag_ce : string_tag_ce;
		if (EG(exception)) {
			zend_string_release(s);
			return;
		}
		object_init_ex(out, ce);
		zend_update_property_str(ce, Z_OBJ_P(out), "value", 5, s);
		zend_string_release(s);
		break;
	}
	case NBT_TAG_LIST: {
		uint8_t t = reader_byte(r);
		int32_t n = (int32_t)reader_int(r), i;
		zval a;
		if (EG(exception))
			return;
		if (n < 0) {
			throw_nbt_data_exception(
				"Array length cannot be less than zero (%d < 0)", n);
			return;
		}
		if (max_depth > 0 && depth + 1 > max_depth) {
			throw_nbt_data_exception(
				"Nesting level too deep: reached max depth of %d tags",
				max_depth);
			return;
		}
		array_init_size(&a, (uint32_t)n);
		if (n && t == NBT_TAG_END) {
			zval_ptr_dtor(&a);
			throw_nbt_data_exception("Unexpected non-empty list of TAG_End");
			return;
		}
		for (i = 0; i < n; i++) {
			zval v;
			decode_tag(r, t, max_depth, depth + 1, &v);
			if (EG(exception)) {
				zval_ptr_dtor(&a);
				return;
			}
			add_next_index_zval(&a, &v);
		}
		object_init_ex(out, list_tag_ce);
		zend_update_property(list_tag_ce, Z_OBJ_P(out), "value", 5, &a);
		zend_update_property_long(list_tag_ce, Z_OBJ_P(out), "tagType", 7, t);
		zval_ptr_dtor(&a);
		break;
	}
	case NBT_TAG_COMPOUND: {
		zval a;
		if (max_depth > 0 && depth + 1 > max_depth) {
			throw_nbt_data_exception(
				"Nesting level too deep: reached max depth of %d tags",
				max_depth);
			return;
		}
		array_init(&a);
		while (!EG(exception)) {
			uint8_t t = reader_byte(r);
			zend_string *name;
			zval child;
			if (EG(exception) || t == NBT_TAG_END)
				break;
			name = reader_string(r);
			if (EG(exception)) {
				zend_string_release(name);
				break;
			}
			decode_tag(r, t, max_depth, depth + 1, &child);
			if (EG(exception)) {
				zend_string_release(name);
				break;
			}
			zend_hash_update(Z_ARRVAL(a), name, &child);
			zend_string_release(name);
		}
		if (EG(exception)) {
			zval_ptr_dtor(&a);
			return;
		}
		object_init_ex(out, compound_tag_ce);
		zend_update_property(compound_tag_ce, Z_OBJ_P(out), "value", 5, &a);
		zval_ptr_dtor(&a);
		break;
	}
	case NBT_TAG_INT_ARRAY: {
		zval a;
		ZVAL_UNDEF(&a);
		reader_int_array(r, &a);
		if (EG(exception)) {
			return;
		}
		object_init_ex(out, int_array_tag_ce);
		zend_update_property(int_array_tag_ce, Z_OBJ_P(out), "value", 5, &a);
		zval_ptr_dtor(&a);
		break;
	}
	default:
		throw_unexpected_tag_type_exception("Unexpected NBT tag type %d", type);
		break;
	}
}
static void encode_tag(FastBufferWriter *w, zval *tag) {
	zval tv;
	if (!tag || Z_TYPE_P(tag) != IS_OBJECT)
		return;
	call_method_0(tag, "getType", &tv);
	if (EG(exception))
		return;
	int type = (int)zval_get_long(&tv);
	zval_ptr_dtor(&tv);
	if (type >= NBT_TAG_BYTE && type <= NBT_TAG_DOUBLE) {
		zval v;
		call_method_0(tag, "getValue", &v);
		if (EG(exception))
			return;
		switch (type) {
		case NBT_TAG_BYTE:
			writer_byte(w, (uint8_t)zval_get_long(&v));
			break;
		case NBT_TAG_SHORT:
			writer_short(w, (uint16_t)zval_get_long(&v));
			break;
		case NBT_TAG_INT:
			writer_int(w, (uint32_t)zval_get_long(&v));
			break;
		case NBT_TAG_LONG:
			writer_long(w, (uint64_t)zval_get_long(&v));
			break;
		case NBT_TAG_FLOAT:
			writer_float(w, (float)zval_get_double(&v));
			break;
		case NBT_TAG_DOUBLE:
			writer_double(w, zval_get_double(&v));
			break;
		}
		zval_ptr_dtor(&v);
		return;
	}
	if (type == NBT_TAG_STRING || type == NBT_TAG_BYTE_ARRAY) {
		zval v;
		call_method_0(tag, "getValue", &v);
		if (EG(exception))
			return;
		if (Z_TYPE(v) == IS_STRING) {
			if (type == NBT_TAG_STRING)
				writer_string(w, Z_STRVAL(v), Z_STRLEN(v));
			else
				writer_bytes(w, Z_STRVAL(v), Z_STRLEN(v));
		}
		zval_ptr_dtor(&v);
		return;
	}
	if (type == NBT_TAG_LIST) {
		zval t, a, *item;
		call_method_0(tag, "getTagType", &t);
		if (EG(exception))
			return;
		writer_byte(w, (uint8_t)zval_get_long(&t));
		zval_ptr_dtor(&t);
		call_method_0(tag, "getValue", &a);
		if (EG(exception))
			return;
		writer_int(w, Z_TYPE(a) == IS_ARRAY
						  ? (uint32_t)zend_hash_num_elements(Z_ARRVAL(a))
						  : 0);
		if (Z_TYPE(a) == IS_ARRAY) {
			ZEND_HASH_FOREACH_VAL(Z_ARRVAL(a), item) {
				encode_tag(w, item);
				if (EG(exception))
					break;
			}
			ZEND_HASH_FOREACH_END();
		}
		zval_ptr_dtor(&a);
		return;
	}
	if (type == NBT_TAG_COMPOUND) {
		zval a, *item;
		zend_string *key;
		zend_ulong index;
		call_method_0(tag, "getValue", &a);
		if (EG(exception))
			return;
		if (Z_TYPE(a) == IS_ARRAY) {
			ZEND_HASH_FOREACH_KEY_VAL(Z_ARRVAL(a), index, key, item) {
				zval ct;
				const char *name;
				size_t len;
				call_method_0(item, "getType", &ct);
				if (EG(exception))
					break;
				writer_byte(w, (uint8_t)zval_get_long(&ct));
				zval_ptr_dtor(&ct);
				if (key) {
					name = ZSTR_VAL(key);
					len = ZSTR_LEN(key);
				} else {
					name = "";
					len = 0;
				}
				writer_string(w, name, len);
				encode_tag(w, item);
				if (EG(exception))
					break;
			}
			ZEND_HASH_FOREACH_END();
		}
		zval_ptr_dtor(&a);
		writer_byte(w, NBT_TAG_END);
		return;
	}
	if (type == NBT_TAG_INT_ARRAY) {
		zval a, *item;
		call_method_0(tag, "getValue", &a);
		if (EG(exception))
			return;
		writer_int(w, Z_TYPE(a) == IS_ARRAY
						  ? (uint32_t)zend_hash_num_elements(Z_ARRVAL(a))
						  : 0);
		if (Z_TYPE(a) == IS_ARRAY) {
			ZEND_HASH_FOREACH_VAL(Z_ARRVAL(a), item) {
				writer_int(w, (uint32_t)zval_get_long(item));
			}
			ZEND_HASH_FOREACH_END();
		}
		zval_ptr_dtor(&a);
	}
}

// BaseNbtSerializer Arginfo & Methods
// -------------------------------------------------------------
ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_serializer_read, 0, 1,
									   pocketmine\\nbt\\TreeRoot, 0)
ZEND_ARG_TYPE_INFO(0, buffer, IS_STRING, 0)
ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(1, offset, IS_LONG, 0, "0")
ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, maxDepth, IS_LONG, 0, "0")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_serializer_read_headless, 0, 2, pocketmine\\nbt\\tag\\Tag, 0)
	ZEND_ARG_TYPE_INFO(0, buffer, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, rootType, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(1, offset, IS_LONG, 0, "0")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, maxDepth, IS_LONG, 0, "0")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_serializer_read_multiple, 0, 1, IS_ARRAY, 0)
	ZEND_ARG_TYPE_INFO(0, buffer, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, maxDepth, IS_LONG, 0, "0")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_serializer_write, 0, 1, IS_STRING, 0)
	ZEND_ARG_OBJ_INFO(0, data, pocketmine\\nbt\\TreeRoot, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_serializer_write_headless, 0, 1, IS_STRING, 0)
	ZEND_ARG_OBJ_INFO(0, data, pocketmine\\nbt\\tag\\Tag, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_serializer_write_multiple, 0, 1, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, data, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_serializer_check_str_len, 0, 1, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, len, IS_LONG, 0)
ZEND_END_ARG_INFO()

// Individual stream arginfos for NbtSerializer methods
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_serializer_read_byte, 0, 0, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_serializer_write_byte, 0, 1, IS_VOID, 0)
	ZEND_ARG_TYPE_INFO(0, v, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_serializer_read_string, 0, 0, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_serializer_write_string, 0, 1, IS_VOID, 0)
	ZEND_ARG_TYPE_INFO(0, v, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_serializer_read_float, 0, 0, IS_DOUBLE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_serializer_write_float, 0, 1, IS_VOID, 0)
	ZEND_ARG_TYPE_INFO(0, v, IS_DOUBLE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_serializer_read_array, 0, 0, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_serializer_write_array, 0, 1, IS_VOID, 0)
	ZEND_ARG_TYPE_INFO(0, array, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_base_serializer_construct, 0, 0, 0)
ZEND_END_ARG_INFO()

PHP_METHOD(pocketmine_nbt_BaseNbtSerializer, __construct) {
	ZEND_PARSE_PARAMETERS_NONE();
	zend_class_entry* binary_stream_ce = (zend_class_entry*)zend_hash_str_find_ptr(CG(class_table), "pocketmine\\utils\\binarystream", sizeof("pocketmine\\utils\\binarystream") - 1);
	if (binary_stream_ce) {
		zval stream_obj;
		object_init_ex(&stream_obj, binary_stream_ce);
		call_method_0(&stream_obj, "__construct", NULL);
		zend_update_property(base_nbt_serializer_ce, Z_OBJ_P(getThis()), "buffer", sizeof("buffer") - 1, &stream_obj);
		zval_ptr_dtor(&stream_obj);
	}
}

PHP_METHOD(pocketmine_nbt_BaseNbtSerializer, read) {
	zend_string* buffer;
	zval* offset_ref = NULL;
	zend_long maxDepth = 0;

	ZEND_PARSE_PARAMETERS_START(1, 3)
		Z_PARAM_STR(buffer)
		Z_PARAM_OPTIONAL
		Z_PARAM_ZVAL(offset_ref)
		Z_PARAM_LONG(maxDepth)
	ZEND_PARSE_PARAMETERS_END();

	size_t offset = 0;
	if (offset_ref) {
		offset = (size_t)zval_get_long(Z_ISREF_P(offset_ref) ? Z_REFVAL_P(offset_ref) : offset_ref);
	}

	NbtSerializerType st = get_serializer_type(getThis());
	FastBufferReader reader = {(const unsigned char*)ZSTR_VAL(buffer), ZSTR_LEN(buffer), &offset, st};

	uint8_t type = reader_byte(&reader);
	if (EG(exception)) RETURN_THROWS();
	if (type == NBT_TAG_END) {
		throw_nbt_data_exception("Found TAG_End at the start of buffer");
		RETURN_THROWS();
	}

	zend_string* root_name = reader_string(&reader);
	if (EG(exception)) {
		zend_string_release(root_name);
		RETURN_THROWS();
	}
	zval root_tag;
	decode_tag(&reader, type, (int)maxDepth, 0, &root_tag);
	if (EG(exception)) {
		zend_string_release(root_name);
		RETURN_THROWS();
	}

	if (offset_ref) {
		if (Z_ISREF_P(offset_ref)) {
			ZVAL_LONG(Z_REFVAL_P(offset_ref), (zend_long)offset);
		} else {
			ZVAL_LONG(offset_ref, (zend_long)offset);
		}
	}

	object_init_ex(return_value, tree_root_ce);
	zend_update_property(tree_root_ce, Z_OBJ_P(return_value), "root", sizeof("root") - 1, &root_tag);
	zend_update_property_str(tree_root_ce, Z_OBJ_P(return_value), "name", sizeof("name") - 1, root_name);
	zend_string_release(root_name);
	zval_ptr_dtor(&root_tag);
}

PHP_METHOD(pocketmine_nbt_BaseNbtSerializer, readHeadless) {
	zend_string* buffer;
	zend_long rootType;
	zval* offset_ref = NULL;
	zend_long maxDepth = 0;

	ZEND_PARSE_PARAMETERS_START(2, 4)
		Z_PARAM_STR(buffer)
		Z_PARAM_LONG(rootType)
		Z_PARAM_OPTIONAL
		Z_PARAM_ZVAL(offset_ref)
		Z_PARAM_LONG(maxDepth)
	ZEND_PARSE_PARAMETERS_END();

	size_t offset = 0;
	if (offset_ref) {
		offset = (size_t)zval_get_long(Z_ISREF_P(offset_ref) ? Z_REFVAL_P(offset_ref) : offset_ref);
	}

	NbtSerializerType st = get_serializer_type(getThis());
	FastBufferReader reader = {(const unsigned char*)ZSTR_VAL(buffer), ZSTR_LEN(buffer), &offset, st};

	decode_tag(&reader, (int)rootType, (int)maxDepth, 0, return_value);
	if (EG(exception)) {
		RETURN_THROWS();
	}

	if (offset_ref) {
		if (Z_ISREF_P(offset_ref)) {
			ZVAL_LONG(Z_REFVAL_P(offset_ref), (zend_long)offset);
		} else {
			ZVAL_LONG(offset_ref, (zend_long)offset);
		}
	}
}

PHP_METHOD(pocketmine_nbt_BaseNbtSerializer, readMultiple) {
	zend_string* buffer;
	zend_long maxDepth = 0;

	ZEND_PARSE_PARAMETERS_START(1, 2)
		Z_PARAM_STR(buffer)
		Z_PARAM_OPTIONAL
		Z_PARAM_LONG(maxDepth)
	ZEND_PARSE_PARAMETERS_END();

	size_t offset = 0;
	NbtSerializerType st = get_serializer_type(getThis());
	FastBufferReader reader = {(const unsigned char*)ZSTR_VAL(buffer), ZSTR_LEN(buffer), &offset, st};

	array_init(return_value);

	while (reader_has_bytes(&reader, 1)) {
		uint8_t type = reader_byte(&reader);
		if (type == NBT_TAG_END) {
			throw_nbt_data_exception("Found TAG_End at the start of buffer");
			zval_ptr_dtor(return_value);
			RETURN_THROWS();
		}

		zend_string* root_name = reader_string(&reader);
		if (EG(exception)) {
			zend_string_release(root_name);
			zval_ptr_dtor(return_value);
			RETURN_THROWS();
		}
		zval root_tag;
		decode_tag(&reader, type, (int)maxDepth, 0, &root_tag);
		if (EG(exception)) {
			zend_string_release(root_name);
			zval_ptr_dtor(return_value);
			RETURN_THROWS();
		}

		zval tree_root;
		object_init_ex(&tree_root, tree_root_ce);
		zend_update_property(tree_root_ce, Z_OBJ(tree_root), "root", sizeof("root") - 1, &root_tag);
		zend_update_property_str(tree_root_ce, Z_OBJ(tree_root), "name", sizeof("name") - 1, root_name);
		zend_string_release(root_name);
		zval_ptr_dtor(&root_tag);

		add_next_index_zval(return_value, &tree_root);
	}
}

PHP_METHOD(pocketmine_nbt_BaseNbtSerializer, write) {
	zval* root;
	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_OBJECT_OF_CLASS(root, tree_root_ce)
	ZEND_PARSE_PARAMETERS_END();

	NbtSerializerType st = get_serializer_type(getThis());
	FastBufferWriter writer = {{0}, st};

	zval tag_zv, name_zv;
	call_method_0(root, "getTag", &tag_zv);
	call_method_0(root, "getName", &name_zv);

	zval type_zv;
	call_method_0(&tag_zv, "getType", &type_zv);

	writer_byte(&writer, (uint8_t)zval_get_long(&type_zv));
	zval_ptr_dtor(&type_zv);

	writer_string(&writer, Z_STRVAL(name_zv), Z_STRLEN(name_zv));
	zval_ptr_dtor(&name_zv);

	encode_tag(&writer, &tag_zv);
	zval_ptr_dtor(&tag_zv);

	smart_str_0(&writer.buffer);
	if (EG(exception)) {
		smart_str_free(&writer.buffer);
		RETURN_THROWS();
	}
	if (writer.buffer.s == NULL) RETURN_EMPTY_STRING();
	RETURN_STR(writer.buffer.s);
}

PHP_METHOD(pocketmine_nbt_BaseNbtSerializer, writeHeadless) {
	zval* tag;
	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_OBJECT_OF_CLASS(tag, tag_ce)
	ZEND_PARSE_PARAMETERS_END();

	NbtSerializerType st = get_serializer_type(getThis());
	FastBufferWriter writer = {{0}, st};

	encode_tag(&writer, tag);

	smart_str_0(&writer.buffer);
	if (EG(exception)) {
		smart_str_free(&writer.buffer);
		RETURN_THROWS();
	}
	if (writer.buffer.s == NULL) RETURN_EMPTY_STRING();
	RETURN_STR(writer.buffer.s);
}

PHP_METHOD(pocketmine_nbt_BaseNbtSerializer, writeMultiple) {
	zval* roots;
	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_ARRAY(roots)
	ZEND_PARSE_PARAMETERS_END();

	NbtSerializerType st = get_serializer_type(getThis());
	FastBufferWriter writer = {{0}, st};

	zval* root;
	ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(roots), root) {
		zval tag_zv, name_zv;
		call_method_0(root, "getTag", &tag_zv);
		call_method_0(root, "getName", &name_zv);

		zval type_zv;
		call_method_0(&tag_zv, "getType", &type_zv);

		writer_byte(&writer, (uint8_t)zval_get_long(&type_zv));
		zval_ptr_dtor(&type_zv);

		writer_string(&writer, Z_STRVAL(name_zv), Z_STRLEN(name_zv));
		zval_ptr_dtor(&name_zv);

		encode_tag(&writer, &tag_zv);
		zval_ptr_dtor(&tag_zv);
	} ZEND_HASH_FOREACH_END();

	smart_str_0(&writer.buffer);
	if (EG(exception)) {
		smart_str_free(&writer.buffer);
		RETURN_THROWS();
	}
	if (writer.buffer.s == NULL) RETURN_EMPTY_STRING();
	RETURN_STR(writer.buffer.s);
}

PHP_METHOD(pocketmine_nbt_BaseNbtSerializer, checkReadStringLength) {
	zend_long len;
	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_LONG(len)
	ZEND_PARSE_PARAMETERS_END();

	if (len > 32767) {
		throw_nbt_data_exception("NBT string length too large (%ld > 32767)", (long)len);
		RETURN_THROWS();
	}
	RETURN_LONG(len);
}

PHP_METHOD(pocketmine_nbt_BaseNbtSerializer, checkWriteStringLength) {
	zend_long len;
	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_LONG(len)
	ZEND_PARSE_PARAMETERS_END();

	if (len > 32767) {
		zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0, "NBT string length too large (%ld > 32767)", (long)len);
		RETURN_THROWS();
	}
	RETURN_LONG(len);
}

// Concrete byte / string stream methods on BaseNbtSerializer
PHP_METHOD(pocketmine_nbt_BaseNbtSerializer, readByte) {
	zval rv;
	zval* buf = zend_read_property(base_nbt_serializer_ce, Z_OBJ_P(getThis()), "buffer", sizeof("buffer") - 1, 1, &rv);
	call_method_0(buf, "getByte", return_value);
}

PHP_METHOD(pocketmine_nbt_BaseNbtSerializer, readSignedByte) {
	zval rv;
	zval* buf = zend_read_property(base_nbt_serializer_ce, Z_OBJ_P(getThis()), "buffer", sizeof("buffer") - 1, 1, &rv);
	zval b;
	call_method_0(buf, "getByte", &b);
	int8_t sb = (int8_t)zval_get_long(&b);
	zval_ptr_dtor(&b);
	RETURN_LONG(sb);
}

PHP_METHOD(pocketmine_nbt_BaseNbtSerializer, writeByte) {
	zend_long v;
	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_LONG(v)
	ZEND_PARSE_PARAMETERS_END();
	zval rv, arg, dummy;
	zval* buf = zend_read_property(base_nbt_serializer_ce, Z_OBJ_P(getThis()), "buffer", sizeof("buffer") - 1, 1, &rv);
	ZVAL_LONG(&arg, v);
	call_method_1(buf, "putByte", &dummy, &arg);
	zval_ptr_dtor(&dummy);
}

PHP_METHOD(pocketmine_nbt_BaseNbtSerializer, readByteArray) {
	zval len_zv;
	call_method_0(getThis(), "readInt", &len_zv);
	zend_long len = zval_get_long(&len_zv);
	zval_ptr_dtor(&len_zv);
	if (len < 0) {
		throw_nbt_data_exception("Array length cannot be less than zero (%ld < 0)", (long)len);
		RETURN_THROWS();
	}
	zval rv, arg;
	zval* buf = zend_read_property(base_nbt_serializer_ce, Z_OBJ_P(getThis()), "buffer", sizeof("buffer") - 1, 1, &rv);
	ZVAL_LONG(&arg, len);
	call_method_1(buf, "get", return_value, &arg);
}

PHP_METHOD(pocketmine_nbt_BaseNbtSerializer, writeByteArray) {
	zend_string* v;
	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_STR(v)
	ZEND_PARSE_PARAMETERS_END();
	zval len_arg, dummy;
	ZVAL_LONG(&len_arg, ZSTR_LEN(v));
	call_method_1(getThis(), "writeInt", &dummy, &len_arg);
	zval_ptr_dtor(&dummy);
	zval rv, str_arg;
	zval* buf = zend_read_property(base_nbt_serializer_ce, Z_OBJ_P(getThis()), "buffer", sizeof("buffer") - 1, 1, &rv);
	ZVAL_STR(&str_arg, v);
	call_method_1(buf, "put", &dummy, &str_arg);
	zval_ptr_dtor(&dummy);
}

PHP_METHOD(pocketmine_nbt_BaseNbtSerializer, readString) {
	zval short_zv;
	call_method_0(getThis(), "readShort", &short_zv);
	zend_long len = zval_get_long(&short_zv);
	zval_ptr_dtor(&short_zv);

	zval len_arg, checked_zv;
	ZVAL_LONG(&len_arg, len);
	call_method_1(getThis(), "checkReadStringLength", &checked_zv, &len_arg);
	if (EG(exception)) {
		RETURN_THROWS();
	}
	zend_long valid_len = zval_get_long(&checked_zv);
	zval_ptr_dtor(&checked_zv);

	zval rv, arg;
	zval* buf = zend_read_property(base_nbt_serializer_ce, Z_OBJ_P(getThis()), "buffer", sizeof("buffer") - 1, 1, &rv);
	ZVAL_LONG(&arg, valid_len);
	call_method_1(buf, "get", return_value, &arg);
}

PHP_METHOD(pocketmine_nbt_BaseNbtSerializer, writeString) {
	zend_string* v;
	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_STR(v)
	ZEND_PARSE_PARAMETERS_END();

	zval len_arg, checked_zv;
	ZVAL_LONG(&len_arg, ZSTR_LEN(v));
	call_method_1(getThis(), "checkWriteStringLength", &checked_zv, &len_arg);
	if (EG(exception)) {
		RETURN_THROWS();
	}
	zval_ptr_dtor(&checked_zv);

	zval dummy;
	call_method_1(getThis(), "writeShort", &dummy, &len_arg);
	zval_ptr_dtor(&dummy);

	zval rv, str_arg;
	zval* buf = zend_read_property(base_nbt_serializer_ce, Z_OBJ_P(getThis()), "buffer", sizeof("buffer") - 1, 1, &rv);
	ZVAL_STR(&str_arg, v);
	call_method_1(buf, "put", &dummy, &str_arg);
	zval_ptr_dtor(&dummy);
}

static const zend_function_entry base_nbt_serializer_methods[] = {
	PHP_ME(pocketmine_nbt_BaseNbtSerializer, __construct, arginfo_base_serializer_construct, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_BaseNbtSerializer, read, arginfo_serializer_read, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_BaseNbtSerializer, readHeadless, arginfo_serializer_read_headless, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_BaseNbtSerializer, readMultiple, arginfo_serializer_read_multiple, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_BaseNbtSerializer, write, arginfo_serializer_write, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_BaseNbtSerializer, writeHeadless, arginfo_serializer_write_headless, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_BaseNbtSerializer, writeMultiple, arginfo_serializer_write_multiple, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_BaseNbtSerializer, checkReadStringLength, arginfo_serializer_check_str_len, ZEND_ACC_PROTECTED | ZEND_ACC_STATIC)
	PHP_ME(pocketmine_nbt_BaseNbtSerializer, checkWriteStringLength, arginfo_serializer_check_str_len, ZEND_ACC_PROTECTED | ZEND_ACC_STATIC)
	PHP_ME(pocketmine_nbt_BaseNbtSerializer, readByte, arginfo_serializer_read_byte, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_BaseNbtSerializer, readSignedByte, arginfo_serializer_read_byte, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_BaseNbtSerializer, writeByte, arginfo_serializer_write_byte, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_BaseNbtSerializer, readByteArray, arginfo_serializer_read_string, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_BaseNbtSerializer, writeByteArray, arginfo_serializer_write_string, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_BaseNbtSerializer, readString, arginfo_serializer_read_string, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_BaseNbtSerializer, writeString, arginfo_serializer_write_string, ZEND_ACC_PUBLIC)
	// Abstract stream methods
	NBT_ABSTRACT_ME(readShort, arginfo_serializer_read_byte, ZEND_ACC_PUBLIC | ZEND_ACC_ABSTRACT)
	NBT_ABSTRACT_ME(readSignedShort, arginfo_serializer_read_byte, ZEND_ACC_PUBLIC | ZEND_ACC_ABSTRACT)
	NBT_ABSTRACT_ME(writeShort, arginfo_serializer_write_byte, ZEND_ACC_PUBLIC | ZEND_ACC_ABSTRACT)
	NBT_ABSTRACT_ME(readInt, arginfo_serializer_read_byte, ZEND_ACC_PUBLIC | ZEND_ACC_ABSTRACT)
	NBT_ABSTRACT_ME(writeInt, arginfo_serializer_write_byte, ZEND_ACC_PUBLIC | ZEND_ACC_ABSTRACT)
	NBT_ABSTRACT_ME(readLong, arginfo_serializer_read_byte, ZEND_ACC_PUBLIC | ZEND_ACC_ABSTRACT)
	NBT_ABSTRACT_ME(writeLong, arginfo_serializer_write_byte, ZEND_ACC_PUBLIC | ZEND_ACC_ABSTRACT)
	NBT_ABSTRACT_ME(readFloat, arginfo_serializer_read_float, ZEND_ACC_PUBLIC | ZEND_ACC_ABSTRACT)
	NBT_ABSTRACT_ME(writeFloat, arginfo_serializer_write_float, ZEND_ACC_PUBLIC | ZEND_ACC_ABSTRACT)
	NBT_ABSTRACT_ME(readDouble, arginfo_serializer_read_float, ZEND_ACC_PUBLIC | ZEND_ACC_ABSTRACT)
	NBT_ABSTRACT_ME(writeDouble, arginfo_serializer_write_float, ZEND_ACC_PUBLIC | ZEND_ACC_ABSTRACT)
	NBT_ABSTRACT_ME(readIntArray, arginfo_serializer_read_array, ZEND_ACC_PUBLIC | ZEND_ACC_ABSTRACT)
	NBT_ABSTRACT_ME(writeIntArray, arginfo_serializer_write_array, ZEND_ACC_PUBLIC | ZEND_ACC_ABSTRACT)
	PHP_FE_END
};

// -------------------------------------------------------------
// BigEndianNbtSerializer Methods
// -------------------------------------------------------------
PHP_METHOD(pocketmine_nbt_BigEndianNbtSerializer, readShort) {
	zval rv;
	zval* buf = zend_read_property(big_endian_nbt_serializer_ce, Z_OBJ_P(getThis()), "buffer", sizeof("buffer") - 1, 1, &rv);
	call_method_0(buf, "getShort", return_value);
}
PHP_METHOD(pocketmine_nbt_BigEndianNbtSerializer, readSignedShort) {
	zval rv;
	zval* buf = zend_read_property(big_endian_nbt_serializer_ce, Z_OBJ_P(getThis()), "buffer", sizeof("buffer") - 1, 1, &rv);
	call_method_0(buf, "getSignedShort", return_value);
}
PHP_METHOD(pocketmine_nbt_BigEndianNbtSerializer, writeShort) {
	zend_long v;
	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_LONG(v)
	ZEND_PARSE_PARAMETERS_END();
	zval rv, arg, dummy;
	zval* buf = zend_read_property(big_endian_nbt_serializer_ce, Z_OBJ_P(getThis()), "buffer", sizeof("buffer") - 1, 1, &rv);
	ZVAL_LONG(&arg, v);
	call_method_1(buf, "putShort", &dummy, &arg);
	zval_ptr_dtor(&dummy);
}
PHP_METHOD(pocketmine_nbt_BigEndianNbtSerializer, readInt) {
	zval rv;
	zval* buf = zend_read_property(big_endian_nbt_serializer_ce, Z_OBJ_P(getThis()), "buffer", sizeof("buffer") - 1, 1, &rv);
	call_method_0(buf, "getInt", return_value);
}
PHP_METHOD(pocketmine_nbt_BigEndianNbtSerializer, writeInt) {
	zend_long v;
	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_LONG(v)
	ZEND_PARSE_PARAMETERS_END();
	zval rv, arg, dummy;
	zval* buf = zend_read_property(big_endian_nbt_serializer_ce, Z_OBJ_P(getThis()), "buffer", sizeof("buffer") - 1, 1, &rv);
	ZVAL_LONG(&arg, v);
	call_method_1(buf, "putInt", &dummy, &arg);
	zval_ptr_dtor(&dummy);
}
PHP_METHOD(pocketmine_nbt_BigEndianNbtSerializer, readLong) {
	zval rv;
	zval* buf = zend_read_property(big_endian_nbt_serializer_ce, Z_OBJ_P(getThis()), "buffer", sizeof("buffer") - 1, 1, &rv);
	call_method_0(buf, "getLong", return_value);
}
PHP_METHOD(pocketmine_nbt_BigEndianNbtSerializer, writeLong) {
	zend_long v;
	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_LONG(v)
	ZEND_PARSE_PARAMETERS_END();
	zval rv, arg, dummy;
	zval* buf = zend_read_property(big_endian_nbt_serializer_ce, Z_OBJ_P(getThis()), "buffer", sizeof("buffer") - 1, 1, &rv);
	ZVAL_LONG(&arg, v);
	call_method_1(buf, "putLong", &dummy, &arg);
	zval_ptr_dtor(&dummy);
}
PHP_METHOD(pocketmine_nbt_BigEndianNbtSerializer, readFloat) {
	zval rv;
	zval* buf = zend_read_property(big_endian_nbt_serializer_ce, Z_OBJ_P(getThis()), "buffer", sizeof("buffer") - 1, 1, &rv);
	call_method_0(buf, "getFloat", return_value);
}
PHP_METHOD(pocketmine_nbt_BigEndianNbtSerializer, writeFloat) {
	double v;
	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_DOUBLE(v)
	ZEND_PARSE_PARAMETERS_END();
	zval rv, arg, dummy;
	zval* buf = zend_read_property(big_endian_nbt_serializer_ce, Z_OBJ_P(getThis()), "buffer", sizeof("buffer") - 1, 1, &rv);
	ZVAL_DOUBLE(&arg, v);
	call_method_1(buf, "putFloat", &dummy, &arg);
	zval_ptr_dtor(&dummy);
}
PHP_METHOD(pocketmine_nbt_BigEndianNbtSerializer, readDouble) {
	zval rv;
	zval* buf = zend_read_property(big_endian_nbt_serializer_ce, Z_OBJ_P(getThis()), "buffer", sizeof("buffer") - 1, 1, &rv);
	call_method_0(buf, "getDouble", return_value);
}
PHP_METHOD(pocketmine_nbt_BigEndianNbtSerializer, writeDouble) {
	double v;
	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_DOUBLE(v)
	ZEND_PARSE_PARAMETERS_END();
	zval rv, arg, dummy;
	zval* buf = zend_read_property(big_endian_nbt_serializer_ce, Z_OBJ_P(getThis()), "buffer", sizeof("buffer") - 1, 1, &rv);
	ZVAL_DOUBLE(&arg, v);
	call_method_1(buf, "putDouble", &dummy, &arg);
	zval_ptr_dtor(&dummy);
}
PHP_METHOD(pocketmine_nbt_BigEndianNbtSerializer, readIntArray) {
	zval len_zv;
	call_method_0(getThis(), "readInt", &len_zv);
	zend_long len = zval_get_long(&len_zv);
	zval_ptr_dtor(&len_zv);
	if (len < 0) {
		throw_nbt_data_exception("Array length cannot be less than zero (%ld < 0)", (long)len);
		RETURN_THROWS();
	}
	array_init(return_value);
	for (zend_long i = 0; i < len; ++i) {
		zval item;
		call_method_0(getThis(), "readInt", &item);
		add_next_index_zval(return_value, &item);
	}
}
PHP_METHOD(pocketmine_nbt_BigEndianNbtSerializer, writeIntArray) {
	zval* array;
	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_ARRAY(array)
	ZEND_PARSE_PARAMETERS_END();
	uint32_t count = zend_hash_num_elements(Z_ARRVAL_P(array));
	zval count_arg, dummy;
	ZVAL_LONG(&count_arg, count);
	call_method_1(getThis(), "writeInt", &dummy, &count_arg);
	zval_ptr_dtor(&dummy);
	zval* entry;
	ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(array), entry) {
		call_method_1(getThis(), "writeInt", &dummy, entry);
		zval_ptr_dtor(&dummy);
	} ZEND_HASH_FOREACH_END();
}

static const zend_function_entry big_endian_nbt_serializer_methods[] = {
	PHP_ME(pocketmine_nbt_BigEndianNbtSerializer, readShort, arginfo_serializer_read_byte, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_BigEndianNbtSerializer, readSignedShort, arginfo_serializer_read_byte, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_BigEndianNbtSerializer, writeShort, arginfo_serializer_write_byte, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_BigEndianNbtSerializer, readInt, arginfo_serializer_read_byte, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_BigEndianNbtSerializer, writeInt, arginfo_serializer_write_byte, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_BigEndianNbtSerializer, readLong, arginfo_serializer_read_byte, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_BigEndianNbtSerializer, writeLong, arginfo_serializer_write_byte, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_BigEndianNbtSerializer, readFloat, arginfo_serializer_read_float, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_BigEndianNbtSerializer, writeFloat, arginfo_serializer_write_float, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_BigEndianNbtSerializer, readDouble, arginfo_serializer_read_float, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_BigEndianNbtSerializer, writeDouble, arginfo_serializer_write_float, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_BigEndianNbtSerializer, readIntArray, arginfo_serializer_read_array, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_BigEndianNbtSerializer, writeIntArray, arginfo_serializer_write_array, ZEND_ACC_PUBLIC)
	PHP_FE_END
};

// -------------------------------------------------------------
// LittleEndianNbtSerializer Methods
// -------------------------------------------------------------
PHP_METHOD(pocketmine_nbt_LittleEndianNbtSerializer, readShort) {
	zval rv;
	zval* buf = zend_read_property(little_endian_nbt_serializer_ce, Z_OBJ_P(getThis()), "buffer", sizeof("buffer") - 1, 1, &rv);
	call_method_0(buf, "getLShort", return_value);
}
PHP_METHOD(pocketmine_nbt_LittleEndianNbtSerializer, readSignedShort) {
	zval rv;
	zval* buf = zend_read_property(little_endian_nbt_serializer_ce, Z_OBJ_P(getThis()), "buffer", sizeof("buffer") - 1, 1, &rv);
	call_method_0(buf, "getSignedLShort", return_value);
}
PHP_METHOD(pocketmine_nbt_LittleEndianNbtSerializer, writeShort) {
	zend_long v;
	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_LONG(v)
	ZEND_PARSE_PARAMETERS_END();
	zval rv, arg, dummy;
	zval* buf = zend_read_property(little_endian_nbt_serializer_ce, Z_OBJ_P(getThis()), "buffer", sizeof("buffer") - 1, 1, &rv);
	ZVAL_LONG(&arg, v);
	call_method_1(buf, "putLShort", &dummy, &arg);
	zval_ptr_dtor(&dummy);
}
PHP_METHOD(pocketmine_nbt_LittleEndianNbtSerializer, readInt) {
	zval rv;
	zval* buf = zend_read_property(little_endian_nbt_serializer_ce, Z_OBJ_P(getThis()), "buffer", sizeof("buffer") - 1, 1, &rv);
	call_method_0(buf, "getLInt", return_value);
}
PHP_METHOD(pocketmine_nbt_LittleEndianNbtSerializer, writeInt) {
	zend_long v;
	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_LONG(v)
	ZEND_PARSE_PARAMETERS_END();
	zval rv, arg, dummy;
	zval* buf = zend_read_property(little_endian_nbt_serializer_ce, Z_OBJ_P(getThis()), "buffer", sizeof("buffer") - 1, 1, &rv);
	ZVAL_LONG(&arg, v);
	call_method_1(buf, "putLInt", &dummy, &arg);
	zval_ptr_dtor(&dummy);
}
PHP_METHOD(pocketmine_nbt_LittleEndianNbtSerializer, readLong) {
	zval rv;
	zval* buf = zend_read_property(little_endian_nbt_serializer_ce, Z_OBJ_P(getThis()), "buffer", sizeof("buffer") - 1, 1, &rv);
	call_method_0(buf, "getLLong", return_value);
}
PHP_METHOD(pocketmine_nbt_LittleEndianNbtSerializer, writeLong) {
	zend_long v;
	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_LONG(v)
	ZEND_PARSE_PARAMETERS_END();
	zval rv, arg, dummy;
	zval* buf = zend_read_property(little_endian_nbt_serializer_ce, Z_OBJ_P(getThis()), "buffer", sizeof("buffer") - 1, 1, &rv);
	ZVAL_LONG(&arg, v);
	call_method_1(buf, "putLLong", &dummy, &arg);
	zval_ptr_dtor(&dummy);
}
PHP_METHOD(pocketmine_nbt_LittleEndianNbtSerializer, readFloat) {
	zval rv;
	zval* buf = zend_read_property(little_endian_nbt_serializer_ce, Z_OBJ_P(getThis()), "buffer", sizeof("buffer") - 1, 1, &rv);
	call_method_0(buf, "getLFloat", return_value);
}
PHP_METHOD(pocketmine_nbt_LittleEndianNbtSerializer, writeFloat) {
	double v;
	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_DOUBLE(v)
	ZEND_PARSE_PARAMETERS_END();
	zval rv, arg, dummy;
	zval* buf = zend_read_property(little_endian_nbt_serializer_ce, Z_OBJ_P(getThis()), "buffer", sizeof("buffer") - 1, 1, &rv);
	ZVAL_DOUBLE(&arg, v);
	call_method_1(buf, "putLFloat", &dummy, &arg);
	zval_ptr_dtor(&dummy);
}
PHP_METHOD(pocketmine_nbt_LittleEndianNbtSerializer, readDouble) {
	zval rv;
	zval* buf = zend_read_property(little_endian_nbt_serializer_ce, Z_OBJ_P(getThis()), "buffer", sizeof("buffer") - 1, 1, &rv);
	call_method_0(buf, "getLDouble", return_value);
}
PHP_METHOD(pocketmine_nbt_LittleEndianNbtSerializer, writeDouble) {
	double v;
	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_DOUBLE(v)
	ZEND_PARSE_PARAMETERS_END();
	zval rv, arg, dummy;
	zval* buf = zend_read_property(little_endian_nbt_serializer_ce, Z_OBJ_P(getThis()), "buffer", sizeof("buffer") - 1, 1, &rv);
	ZVAL_DOUBLE(&arg, v);
	call_method_1(buf, "putLDouble", &dummy, &arg);
	zval_ptr_dtor(&dummy);
}
PHP_METHOD(pocketmine_nbt_LittleEndianNbtSerializer, readIntArray) {
	zval len_zv;
	call_method_0(getThis(), "readInt", &len_zv);
	zend_long len = zval_get_long(&len_zv);
	zval_ptr_dtor(&len_zv);
	if (len < 0) {
		throw_nbt_data_exception("Array length cannot be less than zero (%ld < 0)", (long)len);
		RETURN_THROWS();
	}
	array_init(return_value);
	for (zend_long i = 0; i < len; ++i) {
		zval item;
		call_method_0(getThis(), "readInt", &item);
		add_next_index_zval(return_value, &item);
	}
}
PHP_METHOD(pocketmine_nbt_LittleEndianNbtSerializer, writeIntArray) {
	zval* array;
	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_ARRAY(array)
	ZEND_PARSE_PARAMETERS_END();
	uint32_t count = zend_hash_num_elements(Z_ARRVAL_P(array));
	zval count_arg, dummy;
	ZVAL_LONG(&count_arg, count);
	call_method_1(getThis(), "writeInt", &dummy, &count_arg);
	zval_ptr_dtor(&dummy);
	zval* entry;
	ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(array), entry) {
		call_method_1(getThis(), "writeInt", &dummy, entry);
		zval_ptr_dtor(&dummy);
	} ZEND_HASH_FOREACH_END();
}

static const zend_function_entry little_endian_nbt_serializer_methods[] = {
	PHP_ME(pocketmine_nbt_LittleEndianNbtSerializer, readShort, arginfo_serializer_read_byte, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_LittleEndianNbtSerializer, readSignedShort, arginfo_serializer_read_byte, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_LittleEndianNbtSerializer, writeShort, arginfo_serializer_write_byte, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_LittleEndianNbtSerializer, readInt, arginfo_serializer_read_byte, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_LittleEndianNbtSerializer, writeInt, arginfo_serializer_write_byte, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_LittleEndianNbtSerializer, readLong, arginfo_serializer_read_byte, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_LittleEndianNbtSerializer, writeLong, arginfo_serializer_write_byte, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_LittleEndianNbtSerializer, readFloat, arginfo_serializer_read_float, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_LittleEndianNbtSerializer, writeFloat, arginfo_serializer_write_float, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_LittleEndianNbtSerializer, readDouble, arginfo_serializer_read_float, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_LittleEndianNbtSerializer, writeDouble, arginfo_serializer_write_float, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_LittleEndianNbtSerializer, readIntArray, arginfo_serializer_read_array, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_LittleEndianNbtSerializer, writeIntArray, arginfo_serializer_write_array, ZEND_ACC_PUBLIC)
	PHP_FE_END
};

// -------------------------------------------------------------
// NetworkNbtSerializer Methods (pocketmine\network\mcpe\protocol\serializer)
// -------------------------------------------------------------
PHP_METHOD(pocketmine_network_mcpe_protocol_serializer_NetworkNbtSerializer, readShort) {
	zval rv;
	zval* buf = zend_read_property(network_nbt_serializer_ce, Z_OBJ_P(getThis()), "buffer", sizeof("buffer") - 1, 1, &rv);
	call_method_0(buf, "getLShort", return_value);
}
PHP_METHOD(pocketmine_network_mcpe_protocol_serializer_NetworkNbtSerializer, readSignedShort) {
	zval rv;
	zval* buf = zend_read_property(network_nbt_serializer_ce, Z_OBJ_P(getThis()), "buffer", sizeof("buffer") - 1, 1, &rv);
	call_method_0(buf, "getSignedLShort", return_value);
}
PHP_METHOD(pocketmine_network_mcpe_protocol_serializer_NetworkNbtSerializer, writeShort) {
	zend_long v;
	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_LONG(v)
	ZEND_PARSE_PARAMETERS_END();
	zval rv, arg, dummy;
	zval* buf = zend_read_property(network_nbt_serializer_ce, Z_OBJ_P(getThis()), "buffer", sizeof("buffer") - 1, 1, &rv);
	ZVAL_LONG(&arg, v);
	call_method_1(buf, "putLShort", &dummy, &arg);
	zval_ptr_dtor(&dummy);
}
PHP_METHOD(pocketmine_network_mcpe_protocol_serializer_NetworkNbtSerializer, readInt) {
	zval rv;
	zval* buf = zend_read_property(network_nbt_serializer_ce, Z_OBJ_P(getThis()), "buffer", sizeof("buffer") - 1, 1, &rv);
	call_method_0(buf, "getVarInt", return_value);
}
PHP_METHOD(pocketmine_network_mcpe_protocol_serializer_NetworkNbtSerializer, writeInt) {
	zend_long v;
	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_LONG(v)
	ZEND_PARSE_PARAMETERS_END();
	zval rv, arg, dummy;
	zval* buf = zend_read_property(network_nbt_serializer_ce, Z_OBJ_P(getThis()), "buffer", sizeof("buffer") - 1, 1, &rv);
	ZVAL_LONG(&arg, v);
	call_method_1(buf, "putVarInt", &dummy, &arg);
	zval_ptr_dtor(&dummy);
}
PHP_METHOD(pocketmine_network_mcpe_protocol_serializer_NetworkNbtSerializer, readLong) {
	zval rv;
	zval* buf = zend_read_property(network_nbt_serializer_ce, Z_OBJ_P(getThis()), "buffer", sizeof("buffer") - 1, 1, &rv);
	call_method_0(buf, "getVarLong", return_value);
}
PHP_METHOD(pocketmine_network_mcpe_protocol_serializer_NetworkNbtSerializer, writeLong) {
	zend_long v;
	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_LONG(v)
	ZEND_PARSE_PARAMETERS_END();
	zval rv, arg, dummy;
	zval* buf = zend_read_property(network_nbt_serializer_ce, Z_OBJ_P(getThis()), "buffer", sizeof("buffer") - 1, 1, &rv);
	ZVAL_LONG(&arg, v);
	call_method_1(buf, "putVarLong", &dummy, &arg);
	zval_ptr_dtor(&dummy);
}
PHP_METHOD(pocketmine_network_mcpe_protocol_serializer_NetworkNbtSerializer, readString) {
	zval rv;
	zval* buf = zend_read_property(network_nbt_serializer_ce, Z_OBJ_P(getThis()), "buffer", sizeof("buffer") - 1, 1, &rv);
	zval uvarint_zv;
	call_method_0(buf, "getUnsignedVarInt", &uvarint_zv);
	zend_long len = zval_get_long(&uvarint_zv);
	zval_ptr_dtor(&uvarint_zv);

	zval len_arg, checked_zv;
	ZVAL_LONG(&len_arg, len);
	call_method_1(getThis(), "checkReadStringLength", &checked_zv, &len_arg);
	if (EG(exception)) {
		RETURN_THROWS();
	}
	zend_long valid_len = zval_get_long(&checked_zv);
	zval_ptr_dtor(&checked_zv);

	zval arg;
	ZVAL_LONG(&arg, valid_len);
	call_method_1(buf, "get", return_value, &arg);
}
PHP_METHOD(pocketmine_network_mcpe_protocol_serializer_NetworkNbtSerializer, writeString) {
	zend_string* v;
	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_STR(v)
	ZEND_PARSE_PARAMETERS_END();

	zval len_arg, checked_zv;
	ZVAL_LONG(&len_arg, ZSTR_LEN(v));
	call_method_1(getThis(), "checkWriteStringLength", &checked_zv, &len_arg);
	if (EG(exception)) {
		RETURN_THROWS();
	}
	zend_long valid_len = zval_get_long(&checked_zv);
	zval_ptr_dtor(&checked_zv);

	zval rv, uvarint_arg, dummy;
	zval* buf = zend_read_property(network_nbt_serializer_ce, Z_OBJ_P(getThis()), "buffer", sizeof("buffer") - 1, 1, &rv);
	ZVAL_LONG(&uvarint_arg, valid_len);
	call_method_1(buf, "putUnsignedVarInt", &dummy, &uvarint_arg);
	zval_ptr_dtor(&dummy);

	zval str_arg;
	ZVAL_STR(&str_arg, v);
	call_method_1(buf, "put", &dummy, &str_arg);
	zval_ptr_dtor(&dummy);
}
PHP_METHOD(pocketmine_network_mcpe_protocol_serializer_NetworkNbtSerializer, readFloat) {
	zval rv;
	zval* buf = zend_read_property(network_nbt_serializer_ce, Z_OBJ_P(getThis()), "buffer", sizeof("buffer") - 1, 1, &rv);
	call_method_0(buf, "getLFloat", return_value);
}
PHP_METHOD(pocketmine_network_mcpe_protocol_serializer_NetworkNbtSerializer, writeFloat) {
	double v;
	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_DOUBLE(v)
	ZEND_PARSE_PARAMETERS_END();
	zval rv, arg, dummy;
	zval* buf = zend_read_property(network_nbt_serializer_ce, Z_OBJ_P(getThis()), "buffer", sizeof("buffer") - 1, 1, &rv);
	ZVAL_DOUBLE(&arg, v);
	call_method_1(buf, "putLFloat", &dummy, &arg);
	zval_ptr_dtor(&dummy);
}
PHP_METHOD(pocketmine_network_mcpe_protocol_serializer_NetworkNbtSerializer, readDouble) {
	zval rv;
	zval* buf = zend_read_property(network_nbt_serializer_ce, Z_OBJ_P(getThis()), "buffer", sizeof("buffer") - 1, 1, &rv);
	call_method_0(buf, "getLDouble", return_value);
}
PHP_METHOD(pocketmine_network_mcpe_protocol_serializer_NetworkNbtSerializer, writeDouble) {
	double v;
	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_DOUBLE(v)
	ZEND_PARSE_PARAMETERS_END();
	zval rv, arg, dummy;
	zval* buf = zend_read_property(network_nbt_serializer_ce, Z_OBJ_P(getThis()), "buffer", sizeof("buffer") - 1, 1, &rv);
	ZVAL_DOUBLE(&arg, v);
	call_method_1(buf, "putLDouble", &dummy, &arg);
	zval_ptr_dtor(&dummy);
}
PHP_METHOD(pocketmine_network_mcpe_protocol_serializer_NetworkNbtSerializer, readIntArray) {
	zval len_zv;
	call_method_0(getThis(), "readInt", &len_zv);
	zend_long len = zval_get_long(&len_zv);
	zval_ptr_dtor(&len_zv);
	if (len < 0) {
		throw_nbt_data_exception("Array length cannot be less than zero (%ld < 0)", (long)len);
		RETURN_THROWS();
	}
	array_init(return_value);
	for (zend_long i = 0; i < len; ++i) {
		zval item;
		call_method_0(getThis(), "readInt", &item);
		add_next_index_zval(return_value, &item);
	}
}
PHP_METHOD(pocketmine_network_mcpe_protocol_serializer_NetworkNbtSerializer, writeIntArray) {
	zval* array;
	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_ARRAY(array)
	ZEND_PARSE_PARAMETERS_END();
	uint32_t count = zend_hash_num_elements(Z_ARRVAL_P(array));
	zval count_arg, dummy;
	ZVAL_LONG(&count_arg, count);
	call_method_1(getThis(), "writeInt", &dummy, &count_arg);
	zval_ptr_dtor(&dummy);
	zval* entry;
	ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(array), entry) {
		call_method_1(getThis(), "writeInt", &dummy, entry);
		zval_ptr_dtor(&dummy);
	} ZEND_HASH_FOREACH_END();
}

static const zend_function_entry network_nbt_serializer_methods[] = {
	PHP_ME(pocketmine_network_mcpe_protocol_serializer_NetworkNbtSerializer, readShort, arginfo_serializer_read_byte, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_network_mcpe_protocol_serializer_NetworkNbtSerializer, readSignedShort, arginfo_serializer_read_byte, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_network_mcpe_protocol_serializer_NetworkNbtSerializer, writeShort, arginfo_serializer_write_byte, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_network_mcpe_protocol_serializer_NetworkNbtSerializer, readInt, arginfo_serializer_read_byte, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_network_mcpe_protocol_serializer_NetworkNbtSerializer, writeInt, arginfo_serializer_write_byte, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_network_mcpe_protocol_serializer_NetworkNbtSerializer, readLong, arginfo_serializer_read_byte, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_network_mcpe_protocol_serializer_NetworkNbtSerializer, writeLong, arginfo_serializer_write_byte, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_network_mcpe_protocol_serializer_NetworkNbtSerializer, readString, arginfo_serializer_read_string, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_network_mcpe_protocol_serializer_NetworkNbtSerializer, writeString, arginfo_serializer_write_string, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_network_mcpe_protocol_serializer_NetworkNbtSerializer, readFloat, arginfo_serializer_read_float, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_network_mcpe_protocol_serializer_NetworkNbtSerializer, writeFloat, arginfo_serializer_write_float, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_network_mcpe_protocol_serializer_NetworkNbtSerializer, readDouble, arginfo_serializer_read_float, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_network_mcpe_protocol_serializer_NetworkNbtSerializer, writeDouble, arginfo_serializer_write_float, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_network_mcpe_protocol_serializer_NetworkNbtSerializer, readIntArray, arginfo_serializer_read_array, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_network_mcpe_protocol_serializer_NetworkNbtSerializer, writeIntArray, arginfo_serializer_write_array, ZEND_ACC_PUBLIC)
	PHP_FE_END
};

void register_nbt_serializer_classes() {
	zend_class_entry ce;

	// BaseNbtSerializer (abstract)
	INIT_NS_CLASS_ENTRY(ce, "pocketmine\\nbt", "BaseNbtSerializer", base_nbt_serializer_methods);
	base_nbt_serializer_ce = register_internal_class_with_flags(&ce, NULL, ZEND_ACC_EXPLICIT_ABSTRACT_CLASS);
	zend_class_implements(base_nbt_serializer_ce, 2, nbt_stream_reader_ce, nbt_stream_writer_ce);
	zend_declare_property_null(base_nbt_serializer_ce, "buffer", sizeof("buffer") - 1, ZEND_ACC_PROTECTED);

	// BigEndianNbtSerializer
	INIT_NS_CLASS_ENTRY(ce, "pocketmine\\nbt", "BigEndianNbtSerializer", big_endian_nbt_serializer_methods);
	big_endian_nbt_serializer_ce = zend_register_internal_class_ex(&ce, base_nbt_serializer_ce);

	// LittleEndianNbtSerializer
	INIT_NS_CLASS_ENTRY(ce, "pocketmine\\nbt", "LittleEndianNbtSerializer", little_endian_nbt_serializer_methods);
	little_endian_nbt_serializer_ce = zend_register_internal_class_ex(&ce, base_nbt_serializer_ce);

	// NetworkNbtSerializer (pocketmine\network\mcpe\protocol\serializer\NetworkNbtSerializer)
	INIT_NS_CLASS_ENTRY(ce, "pocketmine\\network\\mcpe\\protocol\\serializer", "NetworkNbtSerializer", network_nbt_serializer_methods);
	network_nbt_serializer_ce = zend_register_internal_class_ex(&ce, base_nbt_serializer_ce);
}
