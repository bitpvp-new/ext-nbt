#include "JsonNbtParser.h"
#include "NbtTags.h"
#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

zend_class_entry *json_nbt_parser_ce = NULL;

typedef struct {
	const char *data;
	size_t length;
	size_t offset;
} JsonStream;

typedef struct {
	char *data;
	size_t length;
	size_t capacity;
} NbtTextBuffer;

static void text_buffer_append(NbtTextBuffer *buffer, char c) {
	if (buffer->length + 1 >= buffer->capacity) {
		buffer->capacity = buffer->capacity == 0 ? 32 : buffer->capacity * 2;
		buffer->data = buffer->data == NULL
						   ? emalloc(buffer->capacity)
						   : erealloc(buffer->data, buffer->capacity);
	}
	buffer->data[buffer->length++] = c;
	buffer->data[buffer->length] = '\0';
}

static void text_buffer_release(NbtTextBuffer *buffer) {
	if (buffer->data != NULL) {
		efree(buffer->data);
	}
	buffer->data = NULL;
	buffer->length = 0;
	buffer->capacity = 0;
}

static zend_bool stream_eof(const JsonStream *stream) {
	return stream->offset >= stream->length;
}

static char stream_get(JsonStream *stream) {
	if (stream_eof(stream)) {
		return '\0';
	}
	return stream->data[stream->offset++];
}

static zend_bool is_numeric_str(const char *value, size_t length) {
	size_t i = 0;
	zend_bool has_digits = 0;
	zend_bool has_dot = 0;
	zend_bool has_exponent = 0;

	if (length == 0) {
		return 0;
	}
	if (value[0] == '+' || value[0] == '-') {
		if (++i == length) {
			return 0;
		}
	}

	for (; i < length; ++i) {
		char c = value[i];
		if (isdigit((unsigned char)c)) {
			has_digits = 1;
		} else if (c == '.') {
			if (has_dot || has_exponent) {
				return 0;
			}
			has_dot = 1;
		} else if (c == 'e' || c == 'E') {
			if (has_exponent || !has_digits) {
				return 0;
			}
			has_exponent = 1;
			has_digits = 0;
			if (i + 1 < length &&
				(value[i + 1] == '+' || value[i + 1] == '-')) {
				++i;
			}
		} else {
			return 0;
		}
	}
	return has_digits;
}

static zend_bool skip_whitespace(JsonStream *stream, char terminator) {
	while (!stream_eof(stream)) {
		char c = stream_get(stream);
		if (c == terminator) {
			return 0;
		}
		if (c == ' ' || c == '\n' || c == '\t' || c == '\r') {
			continue;
		}
		--stream->offset;
		return 1;
	}
	throw_nbt_data_exception("Syntax error: unexpected end of stream, expected "
							 "start of key at offset %zu",
							 stream->offset);
	return 0;
}

static zend_bool read_break(JsonStream *stream, char terminator) {
	size_t offset;
	char c;
	if (stream_eof(stream)) {
		throw_nbt_data_exception("Syntax error: unexpected end of stream, "
								 "expected '%c' at offset %zu",
								 terminator, stream->offset);
		return 0;
	}
	offset = stream->offset;
	c = stream_get(stream);
	if (c == ',') {
		return 0;
	}
	if (c == terminator) {
		return 1;
	}
	throw_nbt_data_exception("Syntax error: unexpected '%c' end at offset %zu",
							 c, offset);
	return 0;
}

static zend_string *read_key(JsonStream *stream) {
	NbtTextBuffer key = {NULL, 0, 0};
	size_t offset = stream->offset;
	zend_bool in_quotes = 0;
	zend_bool found_end = 0;

	while (!stream_eof(stream)) {
		char c = stream_get(stream);
		if (in_quotes) {
			if (c == '"') {
				in_quotes = 0;
				found_end = 1;
			} else if (c == '\\') {
				if (stream_eof(stream)) {
					text_buffer_release(&key);
					throw_nbt_data_exception(
						"Syntax error: unexpected end of stream at offset %zu",
						stream->offset);
					return NULL;
				}
				text_buffer_append(&key, stream_get(stream));
			} else {
				text_buffer_append(&key, c);
			}
		} else {
			if (c == ':') {
				found_end = 1;
				break;
			}
			if (key.length == 0 || found_end) {
				if (c == '\r' || c == '\n' || c == '\t' || c == ' ') {
					continue;
				}
				if (found_end) {
					text_buffer_release(&key);
					throw_nbt_data_exception("Syntax error: unexpected '%c' "
											 "after end of value at offset %zu",
											 c, stream->offset);
					return NULL;
				}
			}
			if (c == '"') {
				if (key.length != 0) {
					text_buffer_release(&key);
					throw_nbt_data_exception(
						"Syntax error: unexpected quote at offset %zu",
						stream->offset - 1);
					return NULL;
				}
				in_quotes = 1;
			} else if (c == '{' || c == '}' || c == '[' || c == ']' ||
					   c == ',') {
				text_buffer_release(&key);
				throw_nbt_data_exception(
					"Syntax error: unexpected '%c' at offset %zu (enclose in "
					"double quotes for literal)",
					c, stream->offset - 1);
				return NULL;
			} else {
				text_buffer_append(&key, c);
			}
		}
	}

	if (key.length == 0) {
		text_buffer_release(&key);
		throw_nbt_data_exception(
			"Syntax error: invalid empty key at offset %zu", offset);
		return NULL;
	}
	if (!found_end) {
		text_buffer_release(&key);
		throw_nbt_data_exception(
			"Syntax error: unexpected end of stream at offset %zu", offset);
		return NULL;
	}
	{
		zend_string *result = zend_string_init(key.data, key.length, 0);
		text_buffer_release(&key);
		return result;
	}
}

static void parse_compound(JsonStream *stream, zval *return_value);
static void parse_list(JsonStream *stream, zval *return_value);

static void read_value(JsonStream *stream, zval *return_value) {
	NbtTextBuffer value = {NULL, 0, 0};
	zend_bool in_quotes = 0;
	size_t offset = stream->offset;
	zend_bool found_end = 0;
	zend_bool has_return_value = 0;

	while (!stream_eof(stream)) {
		char c;
		offset = stream->offset;
		c = stream_get(stream);
		if (in_quotes) {
			if (c == '"') {
				in_quotes = 0;
				object_init_ex(return_value, string_tag_ce);
				zend_update_property_stringl(
					string_tag_ce, Z_OBJ_P(return_value), "value",
					sizeof("value") - 1, value.data != NULL ? value.data : "",
					value.length);
				has_return_value = 1;
				found_end = 1;
			} else if (c == '\\') {
				if (stream_eof(stream)) {
					text_buffer_release(&value);
					throw_nbt_data_exception(
						"Syntax error: unexpected end of stream at offset %zu",
						stream->offset);
					return;
				}
				text_buffer_append(&value, stream_get(stream));
			} else {
				text_buffer_append(&value, c);
			}
		} else {
			if (c == ',' || c == '}' || c == ']') {
				--stream->offset;
				found_end = 1;
				break;
			}
			if (value.length == 0 || found_end) {
				if (c == '\r' || c == '\n' || c == '\t' || c == ' ') {
					continue;
				}
				if (found_end) {
					text_buffer_release(&value);
					throw_nbt_data_exception("Syntax error: unexpected '%c' "
											 "after end of value at offset %zu",
											 c, offset);
					return;
				}
			}
			if (c == '"') {
				if (value.length != 0) {
					text_buffer_release(&value);
					throw_nbt_data_exception(
						"Syntax error: unexpected quote at offset %zu", offset);
					return;
				}
				in_quotes = 1;
			} else if (c == '{') {
				if (value.length != 0) {
					text_buffer_release(&value);
					throw_nbt_data_exception(
						"Syntax error: unexpected compound start at offset %zu "
						"(enclose in double quotes for literal)",
						offset);
					return;
				}
				parse_compound(stream, return_value);
				text_buffer_release(&value);
				if (EG(exception))
					return;
				has_return_value = 1;
				found_end = 1;
			} else if (c == '[') {
				if (value.length != 0) {
					text_buffer_release(&value);
					throw_nbt_data_exception(
						"Syntax error: unexpected list start at offset %zu "
						"(enclose in double quotes for literal)",
						offset);
					return;
				}
				parse_list(stream, return_value);
				text_buffer_release(&value);
				if (EG(exception))
					return;
				has_return_value = 1;
				found_end = 1;
			} else {
				text_buffer_append(&value, c);
			}
		}
	}

	if (has_return_value) {
		text_buffer_release(&value);
		return;
	}
	if (value.length == 0) {
		text_buffer_release(&value);
		throw_nbt_data_exception("Syntax error: empty value at offset %zu",
								 offset);
		return;
	}
	if (!found_end) {
		text_buffer_release(&value);
		throw_nbt_data_exception(
			"Syntax error: unexpected end of stream at offset %zu",
			stream->offset);
		return;
	}

	{
		char suffix = '\0';
		size_t numeric_length = value.length;
		char *numeric_value;
		char last_char =
			(char)tolower((unsigned char)value.data[value.length - 1]);
		if (last_char == 'b' || last_char == 's' || last_char == 'l' ||
			last_char == 'f' || last_char == 'd') {
			suffix = last_char;
			--numeric_length;
		}
		numeric_value = estrndup(value.data, numeric_length);
		if (is_numeric_str(numeric_value, numeric_length)) {
			char *endptr;
			if (suffix == 'f' || suffix == 'd' ||
				memchr(numeric_value, '.', numeric_length) != NULL ||
				memchr(numeric_value, 'e', numeric_length) != NULL ||
				memchr(numeric_value, 'E', numeric_length) != NULL) {
				double number;
				errno = 0;
				number = zend_strtod(numeric_value, &endptr);
				if (errno == ERANGE ||
					endptr != numeric_value + numeric_length ||
					!isfinite(number)) {
					efree(numeric_value);
					text_buffer_release(&value);
					throw_nbt_data_exception(
						"Data error: invalid numeric value at offset %zu",
						stream->offset);
					return;
				}
				object_init_ex(return_value,
							   suffix == 'd' ? double_tag_ce : float_tag_ce);
				zend_update_property_double(suffix == 'd' ? double_tag_ce
														  : float_tag_ce,
											Z_OBJ_P(return_value), "value",
											sizeof("value") - 1, number);
			} else {
				long long number;
				errno = 0;
				number = strtoll(numeric_value, &endptr, 10);
				if (errno == ERANGE ||
					endptr != numeric_value + numeric_length) {
					efree(numeric_value);
					text_buffer_release(&value);
					throw_nbt_data_exception(
						"Data error: integer value is outside the supported "
						"range at offset %zu",
						stream->offset);
					return;
				}
				switch (suffix) {
				case 'b':
					if (number < -128 || number > 127) {
						throw_nbt_data_exception(
							"Data error: Value %lld is outside the allowed "
							"range -128 - 127 at offset %zu",
							number, stream->offset);
						break;
					}
					object_init_ex(return_value, byte_tag_ce);
					zend_update_property_long(
						byte_tag_ce, Z_OBJ_P(return_value), "value",
						sizeof("value") - 1, (zend_long)number);
					break;
				case 's':
					if (number < -32768 || number > 32767) {
						throw_nbt_data_exception(
							"Data error: Value %lld is outside the allowed "
							"range -32768 - 32767 at offset %zu",
							number, stream->offset);
						break;
					}
					object_init_ex(return_value, short_tag_ce);
					zend_update_property_long(
						short_tag_ce, Z_OBJ_P(return_value), "value",
						sizeof("value") - 1, (zend_long)number);
					break;
				case 'l':
					if (number < ZEND_LONG_MIN || number > ZEND_LONG_MAX) {
						throw_nbt_data_exception(
							"Data error: integer value is outside the "
							"supported range at offset %zu",
							stream->offset);
						break;
					}
					object_init_ex(return_value, long_tag_ce);
					zend_update_property_long(
						long_tag_ce, Z_OBJ_P(return_value), "value",
						sizeof("value") - 1, (zend_long)number);
					break;
				default:
					if (number < -2147483648LL || number > 2147483647LL) {
						throw_nbt_data_exception(
							"Data error: Value %lld is outside the allowed "
							"range -2147483648 - 2147483647 at offset %zu",
							number, stream->offset);
						break;
					}
					object_init_ex(return_value, int_tag_ce);
					zend_update_property_long(int_tag_ce, Z_OBJ_P(return_value),
											  "value", sizeof("value") - 1,
											  (zend_long)number);
					break;
				}
			}
		} else {
			if (value.length > 32767) {
				throw_nbt_data_exception("Data error: StringTag cannot hold "
										 "more than 32767 bytes at offset %zu",
										 stream->offset);
			} else {
				object_init_ex(return_value, string_tag_ce);
				zend_update_property_stringl(
					string_tag_ce, Z_OBJ_P(return_value), "value",
					sizeof("value") - 1, value.data != NULL ? value.data : "",
					value.length);
			}
		}
		efree(numeric_value);
	}
	text_buffer_release(&value);
}

static void parse_list(JsonStream *stream, zval *return_value) {
	object_init_ex(return_value, list_tag_ce);
	{
		zval empty_array;
		array_init(&empty_array);
		zend_update_property_long(list_tag_ce, Z_OBJ_P(return_value), "tagType",
								  sizeof("tagType") - 1, NBT_TAG_END);
		zend_update_property(list_tag_ce, Z_OBJ_P(return_value), "value",
							 sizeof("value") - 1, &empty_array);
		zval_ptr_dtor(&empty_array);
	}

	if (skip_whitespace(stream, ']')) {
		while (!stream_eof(stream)) {
			zval value;
			zval rv, type_value;
			zval *expected_type;
			zend_long expected, actual;
			read_value(stream, &value);
			if (EG(exception))
				return;
			expected_type =
				zend_read_property(list_tag_ce, Z_OBJ_P(return_value),
								   "tagType", sizeof("tagType") - 1, 1, &rv);
			expected = zval_get_long(expected_type);
			call_method_0(&value, "getType", &type_value);
			actual = zval_get_long(&type_value);
			zval_ptr_dtor(&type_value);
			if (expected != NBT_TAG_END && expected != actual) {
				zval_ptr_dtor(&value);
				throw_nbt_data_exception("Data error: lists can only contain "
										 "one type of value at offset %zu",
										 stream->offset);
				return;
			}
			call_method_1(return_value, "push", NULL, &value);
			zval_ptr_dtor(&value);
			if (EG(exception))
				return;
			if (read_break(stream, ']'))
				return;
			if (EG(exception))
				return;
		}
		throw_nbt_data_exception(
			"Syntax error: unexpected end of stream at offset %zu",
			stream->offset);
	}
}

static void parse_compound(JsonStream *stream, zval *return_value) {
	object_init_ex(return_value, compound_tag_ce);
	{
		zval empty_array;
		array_init(&empty_array);
		zend_update_property(compound_tag_ce, Z_OBJ_P(return_value), "value",
							 sizeof("value") - 1, &empty_array);
		zval_ptr_dtor(&empty_array);
	}

	if (skip_whitespace(stream, '}')) {
		while (!stream_eof(stream)) {
			zend_string *key = read_key(stream);
			zval rv, value;
			zval *compound_value;
			if (EG(exception))
				return;
			compound_value =
				zend_read_property(compound_tag_ce, Z_OBJ_P(return_value),
								   "value", sizeof("value") - 1, 1, &rv);
			if (zend_hash_exists(Z_ARRVAL_P(compound_value), key)) {
				throw_nbt_data_exception("Syntax error: duplicate compound "
										 "leaf node '%s' at offset %zu",
										 ZSTR_VAL(key), stream->offset);
				zend_string_release(key);
				return;
			}
			read_value(stream, &value);
			if (EG(exception)) {
				zend_string_release(key);
				return;
			}
			{
				zval key_value;
				ZVAL_STR_COPY(&key_value, key);
				call_method_2(return_value, "setTag", NULL, &key_value, &value);
				zval_ptr_dtor(&key_value);
			}
			zend_string_release(key);
			zval_ptr_dtor(&value);
			if (EG(exception))
				return;
			if (read_break(stream, '}'))
				return;
			if (EG(exception))
				return;
		}
		throw_nbt_data_exception(
			"Syntax error: unexpected end of stream at offset %zu",
			stream->offset);
	}
}

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_json_parse, 0, 1,
									   pocketmine\\nbt\\tag\\CompoundTag, 0)
ZEND_ARG_TYPE_INFO(0, data, IS_STRING, 0)
ZEND_END_ARG_INFO()

PHP_METHOD(pocketmine_nbt_JsonNbtParser, parseJson) {
	zend_string *data;
	JsonStream stream;
	ZEND_PARSE_PARAMETERS_START(1, 1)
	Z_PARAM_STR(data)
	ZEND_PARSE_PARAMETERS_END();

	stream.data = ZSTR_VAL(data);
	stream.length = ZSTR_LEN(data);
	stream.offset = 0;
	while (!stream_eof(&stream) && (stream.data[stream.offset] == ' ' ||
									stream.data[stream.offset] == '\r' ||
									stream.data[stream.offset] == '\n' ||
									stream.data[stream.offset] == '\t')) {
		++stream.offset;
	}
	while (stream.length > stream.offset &&
		   (stream.data[stream.length - 1] == ' ' ||
			stream.data[stream.length - 1] == '\r' ||
			stream.data[stream.length - 1] == '\n' ||
			stream.data[stream.length - 1] == '\t')) {
		--stream.length;
	}

	if (stream_eof(&stream) || stream_get(&stream) != '{') {
		char c = stream.length == 0 ? '\0' : stream.data[0];
		throw_nbt_data_exception(
			"Syntax error: expected compound start but got '%c' at offset 1",
			c);
		RETURN_THROWS();
	}
	parse_compound(&stream, return_value);
	if (EG(exception)) {
		RETURN_THROWS();
	}
	if (!stream_eof(&stream)) {
		throw_nbt_data_exception("Syntax error: unexpected trailing characters "
								 "after end of tag: %.*s at offset %zu",
								 (int)(stream.length - stream.offset),
								 stream.data + stream.offset, stream.offset);
		RETURN_THROWS();
	}
}

static const zend_function_entry json_nbt_parser_methods[] = {
	PHP_ME(pocketmine_nbt_JsonNbtParser, parseJson, arginfo_json_parse,
		   ZEND_ACC_PUBLIC | ZEND_ACC_STATIC) PHP_FE_END};

void register_json_nbt_parser_class() {
	zend_class_entry ce;
	INIT_NS_CLASS_ENTRY(ce, "pocketmine\\nbt", "JsonNbtParser",
						json_nbt_parser_methods);
	json_nbt_parser_ce = zend_register_internal_class(&ce);
}
