#ifndef ZEND_UTIL_H
#define ZEND_UTIL_H

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "ext/standard/base64.h"
#include "ext/spl/spl_exceptions.h"
#include "ext/spl/spl_iterators.h"
#include "ext/spl/spl_array.h"
#include "Zend/zend_exceptions.h"
#include "Zend/zend_interfaces.h"
#include "Zend/zend_closures.h"

#define NBT_ABSTRACT_ME(name, arg_info, flags) ZEND_FENTRY(name, NULL, arg_info, flags)

#ifndef ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE
# define ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(pass_by_ref, name, type_hint, allow_null, default_value) \
	ZEND_ARG_TYPE_INFO(pass_by_ref, name, type_hint, allow_null)
#endif

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>

#if PHP_VERSION_ID < 80000
typedef int zend_result;

#ifndef RETURN_THROWS
# define RETURN_THROWS() do { return; } while (0)
#endif
#endif

#if PHP_VERSION_ID >= 80000
#define NBT_CALL_OBJ(zv) ((zv) ? Z_OBJ_P(zv) : NULL)
#else
#define NBT_CALL_OBJ(zv) (zv)
#endif

static inline zend_result call_method_0(zval* obj, const char* method_name, zval* retval) {
	if (!obj || Z_TYPE_P(obj) != IS_OBJECT) {
		if (retval) {
			ZVAL_NULL(retval);
		}
		return FAILURE;
	}
#if PHP_VERSION_ID < 80000
	char lc_name[64];
	size_t len = strlen(method_name);
	if (len < sizeof(lc_name)) {
		for (size_t i = 0; i < len; ++i) {
			lc_name[i] = (char)tolower((unsigned char)method_name[i]);
		}
		lc_name[len] = '\0';
		method_name = lc_name;
	}
#endif
	zval local_rv;
	zval* rv = retval ? retval : &local_rv;
	ZVAL_UNDEF(rv);
	zval* res = zend_call_method(NBT_CALL_OBJ(obj), Z_OBJCE_P(obj), NULL, method_name, strlen(method_name), rv, 0, NULL, NULL);
	if (!retval && Z_TYPE(local_rv) != IS_UNDEF) {
		zval_ptr_dtor(&local_rv);
	}
	return (res != NULL && !EG(exception)) ? SUCCESS : FAILURE;
}

static inline zend_result call_method_1(zval* obj, const char* method_name, zval* retval, zval* arg1) {
	if (!obj || Z_TYPE_P(obj) != IS_OBJECT) {
		if (retval) {
			ZVAL_NULL(retval);
		}
		return FAILURE;
	}
#if PHP_VERSION_ID < 80000
	char lc_name[64];
	size_t len = strlen(method_name);
	if (len < sizeof(lc_name)) {
		for (size_t i = 0; i < len; ++i) {
			lc_name[i] = (char)tolower((unsigned char)method_name[i]);
		}
		lc_name[len] = '\0';
		method_name = lc_name;
	}
#endif
	zval null_arg1;
	if (!arg1) {
		ZVAL_NULL(&null_arg1);
		arg1 = &null_arg1;
	}
	zval local_rv;
	zval* rv = retval ? retval : &local_rv;
	ZVAL_UNDEF(rv);
	zval* res = zend_call_method(NBT_CALL_OBJ(obj), Z_OBJCE_P(obj), NULL, method_name, strlen(method_name), rv, 1, arg1, NULL);
	if (!retval && Z_TYPE(local_rv) != IS_UNDEF) {
		zval_ptr_dtor(&local_rv);
	}
	return (res != NULL && !EG(exception)) ? SUCCESS : FAILURE;
}

static inline zend_result call_method_2(zval* obj, const char* method_name, zval* retval, zval* arg1, zval* arg2) {
	if (!obj || Z_TYPE_P(obj) != IS_OBJECT) {
		if (retval) {
			ZVAL_NULL(retval);
		}
		return FAILURE;
	}
#if PHP_VERSION_ID < 80000
	char lc_name[64];
	size_t len = strlen(method_name);
	if (len < sizeof(lc_name)) {
		for (size_t i = 0; i < len; ++i) {
			lc_name[i] = (char)tolower((unsigned char)method_name[i]);
		}
		lc_name[len] = '\0';
		method_name = lc_name;
	}
#endif
	zval null_arg1, null_arg2;
	if (!arg1) {
		ZVAL_NULL(&null_arg1);
		arg1 = &null_arg1;
	}
	if (!arg2) {
		ZVAL_NULL(&null_arg2);
		arg2 = &null_arg2;
	}
	zval local_rv;
	zval* rv = retval ? retval : &local_rv;
	ZVAL_UNDEF(rv);
	zval* res = zend_call_method(NBT_CALL_OBJ(obj), Z_OBJCE_P(obj), NULL, method_name, strlen(method_name), rv, 2, arg1, arg2);
	if (!retval && Z_TYPE(local_rv) != IS_UNDEF) {
		zval_ptr_dtor(&local_rv);
	}
	return (res != NULL && !EG(exception)) ? SUCCESS : FAILURE;
}

#if PHP_VERSION_ID < 80000
static inline zval* zend_read_property(zend_class_entry *scope, zend_object *object, const char *name, size_t name_length, zend_bool silent, zval *rv) {
	zval zv;
	ZVAL_OBJ(&zv, object);
	return zend_read_property(scope, &zv, name, name_length, silent, rv);
}

static inline void zend_update_property(zend_class_entry *scope, zend_object *object, const char *name, size_t name_length, zval *value) {
	zval zv;
	ZVAL_OBJ(&zv, object);
	zend_update_property(scope, &zv, name, name_length, value);
}

static inline void zend_update_property_long(zend_class_entry *scope, zend_object *object, const char *name, size_t name_length, zend_long value) {
	zval zv;
	ZVAL_OBJ(&zv, object);
	zend_update_property_long(scope, &zv, name, name_length, value);
}

static inline void zend_update_property_double(zend_class_entry *scope, zend_object *object, const char *name, size_t name_length, double value) {
	zval zv;
	ZVAL_OBJ(&zv, object);
	zend_update_property_double(scope, &zv, name, name_length, value);
}

static inline void zend_update_property_str(zend_class_entry *scope, zend_object *object, const char *name, size_t name_length, zend_string *value) {
	zval zv;
	ZVAL_OBJ(&zv, object);
	zend_update_property_str(scope, &zv, name, name_length, value);
}

static inline void zend_update_property_string(zend_class_entry *scope, zend_object *object, const char *name, size_t name_length, const char *value) {
	zval zv;
	ZVAL_OBJ(&zv, object);
	zend_update_property_string(scope, &zv, name, name_length, value);
}

static inline void zend_update_property_stringl(zend_class_entry *scope, zend_object *object, const char *name, size_t name_length, const char *value, size_t value_len) {
	zval zv;
	ZVAL_OBJ(&zv, object);
	zend_update_property_stringl(scope, &zv, name, name_length, value, value_len);
}

static inline void zend_update_property_bool(zend_class_entry *scope, zend_object *object, const char *name, size_t name_length, zend_long value) {
	zval zv;
	ZVAL_OBJ(&zv, object);
	zend_update_property_bool(scope, &zv, name, name_length, value);
}

static inline void zend_update_property_null(zend_class_entry *scope, zend_object *object, const char *name, size_t name_length) {
	zval zv;
	ZVAL_OBJ(&zv, object);
	zend_update_property_null(scope, &zv, name, name_length);
}
#endif

static inline zend_class_entry* register_internal_class_with_flags(zend_class_entry* ce, zend_class_entry* parent, uint32_t flags) {
	ce->ce_flags |= flags;
#if PHP_VERSION_ID >= 80400
	return zend_register_internal_class_with_flags(ce, parent, flags);
#else
	zend_class_entry* registered = zend_register_internal_class_ex(ce, parent);
	if (registered) {
		registered->ce_flags |= flags;
	}
	return registered;
#endif
}

// Exception class entries
extern zend_class_entry* nbt_exception_ce;
extern zend_class_entry* nbt_data_exception_ce;
extern zend_class_entry* no_such_tag_exception_ce;
extern zend_class_entry* unexpected_tag_type_exception_ce;
extern zend_class_entry* invalid_tag_value_exception_ce;

// Interfaces and traits
extern zend_class_entry* nbt_stream_reader_ce;
extern zend_class_entry* nbt_stream_writer_ce;
extern zend_class_entry* integerish_tag_trait_ce;
extern zend_class_entry* no_dynamic_fields_trait_ce;

// Classes
extern zend_class_entry* nbt_ce;
extern zend_class_entry* tree_root_ce;
extern zend_class_entry* reader_tracker_ce;
extern zend_class_entry* base_nbt_serializer_ce;
extern zend_class_entry* big_endian_nbt_serializer_ce;
extern zend_class_entry* little_endian_nbt_serializer_ce;
extern zend_class_entry* network_nbt_serializer_ce;
extern zend_class_entry* json_nbt_parser_ce;

extern zend_class_entry* tag_ce;
extern zend_class_entry* immutable_tag_ce;
extern zend_class_entry* byte_tag_ce;
extern zend_class_entry* short_tag_ce;
extern zend_class_entry* int_tag_ce;
extern zend_class_entry* long_tag_ce;
extern zend_class_entry* float_tag_ce;
extern zend_class_entry* double_tag_ce;
extern zend_class_entry* byte_array_tag_ce;
extern zend_class_entry* string_tag_ce;
extern zend_class_entry* int_array_tag_ce;
extern zend_class_entry* list_tag_ce;
extern zend_class_entry* compound_tag_ce;

void throw_nbt_exception(const char* format, ...);
void throw_nbt_data_exception(const char* format, ...);
void throw_no_such_tag_exception(const char* format, ...);
void throw_unexpected_tag_type_exception(const char* format, ...);
void throw_invalid_tag_value_exception(const char* format, ...);

#endif /* ZEND_UTIL_H */
