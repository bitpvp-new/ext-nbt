#include "NbtTags.h"
#include "Zend/zend_smart_str.h"
#include <math.h>

// Global class entries
zend_class_entry* nbt_exception_ce = NULL;
zend_class_entry* nbt_data_exception_ce = NULL;
zend_class_entry* no_such_tag_exception_ce = NULL;
zend_class_entry* unexpected_tag_type_exception_ce = NULL;
zend_class_entry* invalid_tag_value_exception_ce = NULL;

zend_class_entry* nbt_stream_reader_ce = NULL;
zend_class_entry* nbt_stream_writer_ce = NULL;
zend_class_entry* integerish_tag_trait_ce = NULL;
zend_class_entry* no_dynamic_fields_trait_ce = NULL;

zend_class_entry* nbt_ce = NULL;
zend_class_entry* tree_root_ce = NULL;
zend_class_entry* reader_tracker_ce = NULL;

zend_class_entry* tag_ce = NULL;
zend_class_entry* immutable_tag_ce = NULL;
zend_class_entry* byte_tag_ce = NULL;
zend_class_entry* short_tag_ce = NULL;
zend_class_entry* int_tag_ce = NULL;
zend_class_entry* long_tag_ce = NULL;
zend_class_entry* float_tag_ce = NULL;
zend_class_entry* double_tag_ce = NULL;
zend_class_entry* byte_array_tag_ce = NULL;
zend_class_entry* string_tag_ce = NULL;
zend_class_entry* int_array_tag_ce = NULL;
zend_class_entry* list_tag_ce = NULL;
zend_class_entry* compound_tag_ce = NULL;

// Exception throwing helpers
void throw_nbt_exception(const char* format, ...) {
	char buf[1024];
	va_list args;
	va_start(args, format);
	vsnprintf(buf, sizeof(buf), format, args);
	va_end(args);
	zend_throw_exception(nbt_exception_ce, buf, 0);
}

void throw_nbt_data_exception(const char* format, ...) {
	char buf[1024];
	va_list args;
	va_start(args, format);
	vsnprintf(buf, sizeof(buf), format, args);
	va_end(args);
	zend_throw_exception(nbt_data_exception_ce, buf, 0);
}

void throw_no_such_tag_exception(const char* format, ...) {
	char buf[1024];
	va_list args;
	va_start(args, format);
	vsnprintf(buf, sizeof(buf), format, args);
	va_end(args);
	zend_throw_exception(no_such_tag_exception_ce, buf, 0);
}

void throw_unexpected_tag_type_exception(const char* format, ...) {
	char buf[1024];
	va_list args;
	va_start(args, format);
	vsnprintf(buf, sizeof(buf), format, args);
	va_end(args);
	zend_throw_exception(unexpected_tag_type_exception_ce, buf, 0);
}

void throw_invalid_tag_value_exception(const char* format, ...) {
	char buf[1024];
	va_list args;
	va_start(args, format);
	vsnprintf(buf, sizeof(buf), format, args);
	va_end(args);
	zend_throw_exception(invalid_tag_value_exception_ce, buf, 0);
}

static inline void check_arg_count(const char* method_name, uint32_t num_args, uint32_t max_args) {
	if (num_args > max_args) {
		zend_throw_error(zend_ce_argument_count_error, "%s() expects at most %u parameters, %u given", method_name, max_args, num_args);
	}
}

// -------------------------------------------------------------
// Exception registration
// -------------------------------------------------------------
void register_nbt_exceptions() {
	zend_class_entry ce;

	INIT_NS_CLASS_ENTRY(ce, "pocketmine\\nbt", "NbtException", NULL);
	nbt_exception_ce = zend_register_internal_class_ex(&ce, spl_ce_RuntimeException);

	INIT_NS_CLASS_ENTRY(ce, "pocketmine\\nbt", "NbtDataException", NULL);
	nbt_data_exception_ce = zend_register_internal_class_ex(&ce, nbt_exception_ce);

	INIT_NS_CLASS_ENTRY(ce, "pocketmine\\nbt", "NoSuchTagException", NULL);
	no_such_tag_exception_ce = zend_register_internal_class_ex(&ce, nbt_exception_ce);

	INIT_NS_CLASS_ENTRY(ce, "pocketmine\\nbt", "UnexpectedTagTypeException", NULL);
	unexpected_tag_type_exception_ce = zend_register_internal_class_ex(&ce, nbt_exception_ce);

	INIT_NS_CLASS_ENTRY(ce, "pocketmine\\nbt", "InvalidTagValueException", NULL);
	invalid_tag_value_exception_ce = register_internal_class_with_flags(&ce, spl_ce_InvalidArgumentException, ZEND_ACC_FINAL);
}

// -------------------------------------------------------------
// Interfaces and Traits
// -------------------------------------------------------------

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_read_byte, 0, 0, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_read_float, 0, 0, IS_DOUBLE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_read_string, 0, 0, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_read_array, 0, 0, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

static const zend_function_entry nbt_stream_reader_methods[] = {
	NBT_ABSTRACT_ME(readByte, arginfo_read_byte, ZEND_ACC_PUBLIC | ZEND_ACC_ABSTRACT)
	NBT_ABSTRACT_ME(readSignedByte, arginfo_read_byte, ZEND_ACC_PUBLIC | ZEND_ACC_ABSTRACT)
	NBT_ABSTRACT_ME(readShort, arginfo_read_byte, ZEND_ACC_PUBLIC | ZEND_ACC_ABSTRACT)
	NBT_ABSTRACT_ME(readSignedShort, arginfo_read_byte, ZEND_ACC_PUBLIC | ZEND_ACC_ABSTRACT)
	NBT_ABSTRACT_ME(readInt, arginfo_read_byte, ZEND_ACC_PUBLIC | ZEND_ACC_ABSTRACT)
	NBT_ABSTRACT_ME(readLong, arginfo_read_byte, ZEND_ACC_PUBLIC | ZEND_ACC_ABSTRACT)
	NBT_ABSTRACT_ME(readFloat, arginfo_read_float, ZEND_ACC_PUBLIC | ZEND_ACC_ABSTRACT)
	NBT_ABSTRACT_ME(readDouble, arginfo_read_float, ZEND_ACC_PUBLIC | ZEND_ACC_ABSTRACT)
	NBT_ABSTRACT_ME(readByteArray, arginfo_read_string, ZEND_ACC_PUBLIC | ZEND_ACC_ABSTRACT)
	NBT_ABSTRACT_ME(readString, arginfo_read_string, ZEND_ACC_PUBLIC | ZEND_ACC_ABSTRACT)
	NBT_ABSTRACT_ME(readIntArray, arginfo_read_array, ZEND_ACC_PUBLIC | ZEND_ACC_ABSTRACT)
	PHP_FE_END
};

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_write_byte, 0, 1, IS_VOID, 0)
	ZEND_ARG_TYPE_INFO(0, v, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_write_float, 0, 1, IS_VOID, 0)
	ZEND_ARG_TYPE_INFO(0, v, IS_DOUBLE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_write_string, 0, 1, IS_VOID, 0)
	ZEND_ARG_TYPE_INFO(0, v, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_write_array, 0, 1, IS_VOID, 0)
	ZEND_ARG_TYPE_INFO(0, array, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

static const zend_function_entry nbt_stream_writer_methods[] = {
	NBT_ABSTRACT_ME(writeByte, arginfo_write_byte, ZEND_ACC_PUBLIC | ZEND_ACC_ABSTRACT)
	NBT_ABSTRACT_ME(writeShort, arginfo_write_byte, ZEND_ACC_PUBLIC | ZEND_ACC_ABSTRACT)
	NBT_ABSTRACT_ME(writeInt, arginfo_write_byte, ZEND_ACC_PUBLIC | ZEND_ACC_ABSTRACT)
	NBT_ABSTRACT_ME(writeLong, arginfo_write_byte, ZEND_ACC_PUBLIC | ZEND_ACC_ABSTRACT)
	NBT_ABSTRACT_ME(writeFloat, arginfo_write_float, ZEND_ACC_PUBLIC | ZEND_ACC_ABSTRACT)
	NBT_ABSTRACT_ME(writeDouble, arginfo_write_float, ZEND_ACC_PUBLIC | ZEND_ACC_ABSTRACT)
	NBT_ABSTRACT_ME(writeByteArray, arginfo_write_string, ZEND_ACC_PUBLIC | ZEND_ACC_ABSTRACT)
	NBT_ABSTRACT_ME(writeString, arginfo_write_string, ZEND_ACC_PUBLIC | ZEND_ACC_ABSTRACT)
	NBT_ABSTRACT_ME(writeIntArray, arginfo_write_array, ZEND_ACC_PUBLIC | ZEND_ACC_ABSTRACT)
	PHP_FE_END
};

static void throw_dynamic_field_exception(zval* obj, zend_string* field) {
	zend_throw_exception_ex(spl_ce_RuntimeException, 0,
		"Cannot access dynamic field \"%s\": Dynamic field access on %s is no longer supported",
		ZSTR_VAL(field), ZSTR_VAL(Z_OBJCE_P(obj)->name));
}

ZEND_BEGIN_ARG_INFO_EX(arginfo_dynamic_field_get, 0, 0, 1)
	ZEND_ARG_INFO(0, name)
ZEND_END_ARG_INFO()

PHP_METHOD(pocketmine_nbt_tag_NoDynamicFieldsTrait, __get) {
	zend_string* name;
	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_STR(name)
	ZEND_PARSE_PARAMETERS_END();
	throw_dynamic_field_exception(getThis(), name);
}

ZEND_BEGIN_ARG_INFO_EX(arginfo_dynamic_field_set, 0, 0, 2)
	ZEND_ARG_INFO(0, name)
	ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

PHP_METHOD(pocketmine_nbt_tag_NoDynamicFieldsTrait, __set) {
	zend_string* name;
	zval* value;
	ZEND_PARSE_PARAMETERS_START(2, 2)
		Z_PARAM_STR(name)
		Z_PARAM_ZVAL(value)
	ZEND_PARSE_PARAMETERS_END();
	throw_dynamic_field_exception(getThis(), name);
}

PHP_METHOD(pocketmine_nbt_tag_NoDynamicFieldsTrait, __isset) {
	zend_string* name;
	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_STR(name)
	ZEND_PARSE_PARAMETERS_END();
	throw_dynamic_field_exception(getThis(), name);
}

PHP_METHOD(pocketmine_nbt_tag_NoDynamicFieldsTrait, __unset) {
	zend_string* name;
	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_STR(name)
	ZEND_PARSE_PARAMETERS_END();
	throw_dynamic_field_exception(getThis(), name);
}

static const zend_function_entry no_dynamic_fields_trait_methods[] = {
	PHP_ME(pocketmine_nbt_tag_NoDynamicFieldsTrait, __get, arginfo_dynamic_field_get, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_tag_NoDynamicFieldsTrait, __set, arginfo_dynamic_field_set, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_tag_NoDynamicFieldsTrait, __isset, arginfo_dynamic_field_get, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_tag_NoDynamicFieldsTrait, __unset, arginfo_dynamic_field_get, ZEND_ACC_PUBLIC)
	PHP_FE_END
};

PHP_METHOD(pocketmine_nbt_tag_IntegerishTagTrait, getValue) {
	ZEND_PARSE_PARAMETERS_NONE();
	zval rv;
	zval* val = zend_read_property(Z_OBJCE_P(getThis()), Z_OBJ_P(getThis()), "value", sizeof("value") - 1, 1, &rv);
	RETURN_ZVAL(val, 1, 0);
}

PHP_METHOD(pocketmine_nbt_tag_IntegerishTagTrait, stringifyValue) {
	zend_long indentation = 0;
	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_LONG(indentation)
	ZEND_PARSE_PARAMETERS_END();

	zval rv;
	zval* val = zend_read_property(Z_OBJCE_P(getThis()), Z_OBJ_P(getThis()), "value", sizeof("value") - 1, 1, &rv);
	convert_to_string(val);
	RETURN_ZVAL(val, 1, 0);
}

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_integerish_get_value, 0, 0, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_stringify_value, 0, 1, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, indentation, IS_LONG, 0)
ZEND_END_ARG_INFO()

static const zend_function_entry integerish_tag_trait_methods[] = {
	NBT_ABSTRACT_ME(min, arginfo_integerish_get_value, ZEND_ACC_PROTECTED | ZEND_ACC_ABSTRACT)
	NBT_ABSTRACT_ME(max, arginfo_integerish_get_value, ZEND_ACC_PROTECTED | ZEND_ACC_ABSTRACT)
	PHP_ME(pocketmine_nbt_tag_IntegerishTagTrait, getValue, arginfo_integerish_get_value, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_tag_IntegerishTagTrait, stringifyValue, arginfo_stringify_value, ZEND_ACC_PROTECTED)
	PHP_FE_END
};

void register_nbt_interfaces_traits() {
	zend_class_entry ce;

	INIT_NS_CLASS_ENTRY(ce, "pocketmine\\nbt", "NbtStreamReader", nbt_stream_reader_methods);
	nbt_stream_reader_ce = zend_register_internal_interface(&ce);

	INIT_NS_CLASS_ENTRY(ce, "pocketmine\\nbt", "NbtStreamWriter", nbt_stream_writer_methods);
	nbt_stream_writer_ce = zend_register_internal_interface(&ce);

	INIT_NS_CLASS_ENTRY(ce, "pocketmine\\nbt\\tag", "NoDynamicFieldsTrait", no_dynamic_fields_trait_methods);
	no_dynamic_fields_trait_ce = register_internal_class_with_flags(&ce, NULL, ZEND_ACC_TRAIT);

	INIT_NS_CLASS_ENTRY(ce, "pocketmine\\nbt\\tag", "IntegerishTagTrait", integerish_tag_trait_methods);
	integerish_tag_trait_ce = register_internal_class_with_flags(&ce, NULL, ZEND_ACC_TRAIT);
	zend_declare_property_null(integerish_tag_trait_ce, "value", sizeof("value") - 1, ZEND_ACC_PRIVATE);
}

// -------------------------------------------------------------
// Tag Base Class
// -------------------------------------------------------------

ZEND_BEGIN_ARG_INFO_EX(arginfo_tag_get_value, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_tag_get_type, 0, 0, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_tag_write, 0, 1, IS_VOID, 0)
	ZEND_ARG_OBJ_INFO(0, writer, pocketmine\\nbt\\NbtStreamWriter, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_tag_magic_to_string, 0, 0, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_tag_to_string, 0, 0, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, indentation, IS_LONG, 0, "0")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_tag_get_type_name, 0, 0, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_tag_safe_clone, 0, 0, pocketmine\\nbt\\tag\\Tag, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_tag_make_copy, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_tag_equals, 0, 1, _IS_BOOL, 0)
	ZEND_ARG_OBJ_INFO(0, that, pocketmine\\nbt\\tag\\Tag, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_tag_restrict_arg_count, 0, 3, IS_VOID, 0)
	ZEND_ARG_TYPE_INFO(0, func, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, haveArgs, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, wantMaxArgs, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_tag_clone, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_tag_get_iterator, 0, 0, Traversable, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_compound_construct, 0, 0, 0)
ZEND_END_ARG_INFO()

PHP_METHOD(pocketmine_nbt_tag_Tag, toString) {
	zend_long indentation = 0;
	ZEND_PARSE_PARAMETERS_START(0, 1)
		Z_PARAM_OPTIONAL
		Z_PARAM_LONG(indentation)
	ZEND_PARSE_PARAMETERS_END();

	zval type_name_zv, stringified_zv;
	call_method_0(getThis(), "getTypeName", &type_name_zv);
	if (EG(exception)) return;

	zval param;
	ZVAL_LONG(&param, indentation);
	call_method_1(getThis(), "stringifyValue", &stringified_zv, &param);
	if (EG(exception)) {
		zval_ptr_dtor(&type_name_zv);
		return;
	}

	smart_str res = {0};
	smart_str_appends(&res, "TAG_");
	smart_str_appendl(&res, Z_STRVAL(type_name_zv), Z_STRLEN(type_name_zv));
	smart_str_appendc(&res, '=');
	smart_str_appendl(&res, Z_STRVAL(stringified_zv), Z_STRLEN(stringified_zv));
	zval_ptr_dtor(&type_name_zv);
	zval_ptr_dtor(&stringified_zv);

	smart_str_0(&res);
	RETURN_STR(res.s);
}

PHP_METHOD(pocketmine_nbt_tag_Tag, __toString) {
	ZEND_PARSE_PARAMETERS_NONE();
	call_method_0(getThis(), "toString", return_value);
}

PHP_METHOD(pocketmine_nbt_tag_Tag, safeClone) {
	ZEND_PARSE_PARAMETERS_NONE();

	zval rv;
	zval* cloning = zend_read_property(tag_ce, Z_OBJ_P(getThis()), "cloning", sizeof("cloning") - 1, 1, &rv);

	if (zend_is_true(cloning)) {
		zend_throw_exception(spl_ce_RuntimeException, "Recursive NBT tag dependency detected", 0);
		RETURN_THROWS();
	}

	zend_update_property_bool(tag_ce, Z_OBJ_P(getThis()), "cloning", sizeof("cloning") - 1, 1);

	zval copy_zv;
	call_method_0(getThis(), "makeCopy", &copy_zv);

	zend_update_property_bool(tag_ce, Z_OBJ_P(getThis()), "cloning", sizeof("cloning") - 1, 0);

	if (EG(exception)) {
		zval_ptr_dtor(&copy_zv);
		RETURN_THROWS();
	}

	if (Z_TYPE(copy_zv) == IS_OBJECT) {
		zend_update_property_bool(tag_ce, Z_OBJ(copy_zv), "cloning", sizeof("cloning") - 1, 0);
	}

	RETURN_ZVAL(&copy_zv, 0, 0);
}

PHP_METHOD(pocketmine_nbt_tag_Tag, makeCopy) {
	ZEND_PARSE_PARAMETERS_NONE();
	zend_object* new_obj = Z_OBJ_HT_P(getThis())->clone_obj(NBT_CALL_OBJ(getThis()));
	RETURN_OBJ(new_obj);
}

PHP_METHOD(pocketmine_nbt_tag_Tag, equals) {
	zval* that;
	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_OBJECT_OF_CLASS(that, tag_ce)
	ZEND_PARSE_PARAMETERS_END();

	if (!instanceof_function(Z_OBJCE_P(that), Z_OBJCE_P(getThis()))) {
		RETURN_FALSE;
	}

	zval v1, v2;
	call_method_0(getThis(), "getValue", &v1);
	call_method_0(that, "getValue", &v2);

	zend_bool eq = fast_is_identical_function(&v1, &v2);
	zval_ptr_dtor(&v1);
	zval_ptr_dtor(&v2);

	RETURN_BOOL(eq);
}

PHP_METHOD(pocketmine_nbt_tag_Tag, restrictArgCount) {
	zend_string* func;
	zend_long haveArgs, wantMaxArgs;
	ZEND_PARSE_PARAMETERS_START(3, 3)
		Z_PARAM_STR(func)
		Z_PARAM_LONG(haveArgs)
		Z_PARAM_LONG(wantMaxArgs)
	ZEND_PARSE_PARAMETERS_END();

	if (haveArgs > wantMaxArgs) {
		zend_throw_error(zend_ce_argument_count_error, "%s() expects at most %ld parameters, %ld given", ZSTR_VAL(func), (long)wantMaxArgs, (long)haveArgs);
	}
}

static const zend_function_entry tag_methods[] = {
	NBT_ABSTRACT_ME(getValue, arginfo_tag_get_value, ZEND_ACC_PUBLIC | ZEND_ACC_ABSTRACT)
	NBT_ABSTRACT_ME(getType, arginfo_tag_get_type, ZEND_ACC_PUBLIC | ZEND_ACC_ABSTRACT)
	NBT_ABSTRACT_ME(write, arginfo_tag_write, ZEND_ACC_PUBLIC | ZEND_ACC_ABSTRACT)
	PHP_ME(pocketmine_nbt_tag_Tag, __toString, arginfo_tag_magic_to_string, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_tag_Tag, toString, arginfo_tag_to_string, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	NBT_ABSTRACT_ME(getTypeName, arginfo_tag_get_type_name, ZEND_ACC_PROTECTED | ZEND_ACC_ABSTRACT)
	NBT_ABSTRACT_ME(stringifyValue, arginfo_stringify_value, ZEND_ACC_PROTECTED | ZEND_ACC_ABSTRACT)
	PHP_ME(pocketmine_nbt_tag_Tag, safeClone, arginfo_tag_safe_clone, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_tag_Tag, makeCopy, arginfo_tag_make_copy, ZEND_ACC_PROTECTED)
	PHP_ME(pocketmine_nbt_tag_Tag, equals, arginfo_tag_equals, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_tag_Tag, restrictArgCount, arginfo_tag_restrict_arg_count, ZEND_ACC_PROTECTED | ZEND_ACC_STATIC)
	PHP_FE_END
};

// -------------------------------------------------------------
// ImmutableTag
// -------------------------------------------------------------
PHP_METHOD(pocketmine_nbt_tag_ImmutableTag, makeCopy) {
	ZEND_PARSE_PARAMETERS_NONE();
	RETURN_ZVAL(getThis(), 1, 0);
}

static const zend_function_entry immutable_tag_methods[] = {
	PHP_ME(pocketmine_nbt_tag_ImmutableTag, makeCopy, arginfo_tag_make_copy, ZEND_ACC_PROTECTED)
	PHP_FE_END
};

// Helper macro for integer tags
#define DEFINE_INTEGER_TAG_CLASS(ClassUpper, ClassName, TagNameStr, TagTypeCode, MinVal, MaxVal, ReadMethod, WriteMethod) \
PHP_METHOD(pocketmine_nbt_tag_##ClassName, __construct) { \
	zend_long value; \
	ZEND_PARSE_PARAMETERS_START(1, -1) \
		Z_PARAM_LONG(value) \
	ZEND_PARSE_PARAMETERS_END(); \
	check_arg_count("pocketmine\\nbt\\tag\\" #ClassName "::__construct", EX(func)->common.num_args, 1); \
	if (value < (MinVal) || value > (MaxVal)) { \
		throw_invalid_tag_value_exception("Value %ld is outside the allowed range %ld - %ld", (long)value, (long)(MinVal), (long)(MaxVal)); \
		RETURN_THROWS(); \
	} \
	zend_update_property_long(ClassUpper##_tag_ce, \
		Z_OBJ_P(getThis()), \
		"value", sizeof("value") - 1, value); \
} \
PHP_METHOD(pocketmine_nbt_tag_##ClassName, min) { \
	ZEND_PARSE_PARAMETERS_NONE(); \
	RETURN_LONG(MinVal); \
} \
PHP_METHOD(pocketmine_nbt_tag_##ClassName, max) { \
	ZEND_PARSE_PARAMETERS_NONE(); \
	RETURN_LONG(MaxVal); \
} \
PHP_METHOD(pocketmine_nbt_tag_##ClassName, getTypeName) { \
	ZEND_PARSE_PARAMETERS_NONE(); \
	RETURN_STRING(TagNameStr); \
} \
PHP_METHOD(pocketmine_nbt_tag_##ClassName, getType) { \
	ZEND_PARSE_PARAMETERS_NONE(); \
	RETURN_LONG(TagTypeCode); \
} \
PHP_METHOD(pocketmine_nbt_tag_##ClassName, read) { \
	zval* reader; \
	ZEND_PARSE_PARAMETERS_START(1, 1) \
		Z_PARAM_OBJECT_OF_CLASS(reader, nbt_stream_reader_ce) \
	ZEND_PARSE_PARAMETERS_END(); \
	zval val_zv; \
	call_method_0(reader, #ReadMethod, &val_zv); \
	object_init_ex(return_value, ClassUpper##_tag_ce); \
	call_method_1(return_value, "__construct", NULL, &val_zv); \
	zval_ptr_dtor(&val_zv); \
} \
PHP_METHOD(pocketmine_nbt_tag_##ClassName, write) { \
	zval* writer; \
	ZEND_PARSE_PARAMETERS_START(1, 1) \
		Z_PARAM_OBJECT_OF_CLASS(writer, nbt_stream_writer_ce) \
	ZEND_PARSE_PARAMETERS_END(); \
	zval rv; \
	zval* val = zend_read_property(ClassUpper##_tag_ce, \
		Z_OBJ_P(getThis()), \
		"value", sizeof("value") - 1, 1, &rv); \
	zval dummy; \
	call_method_1(writer, #WriteMethod, &dummy, val); \
	zval_ptr_dtor(&dummy); \
}

DEFINE_INTEGER_TAG_CLASS(byte, ByteTag, "Byte", NBT_TAG_BYTE, -128, 127, readSignedByte, writeByte)
DEFINE_INTEGER_TAG_CLASS(short, ShortTag, "Short", NBT_TAG_SHORT, -32768, 32767, readSignedShort, writeShort)
DEFINE_INTEGER_TAG_CLASS(int, IntTag, "Int", NBT_TAG_INT, -2147483647L - 1, 2147483647L, readInt, writeInt)
DEFINE_INTEGER_TAG_CLASS(long, LongTag, "Long", NBT_TAG_LONG, ZEND_LONG_MIN, ZEND_LONG_MAX, readLong, writeLong)

ZEND_BEGIN_ARG_INFO_EX(arginfo_integerish_construct, 0, 0, 1)
	ZEND_ARG_TYPE_INFO(0, value, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_read_tag, 0, 0, 1)
	ZEND_ARG_OBJ_INFO(0, reader, pocketmine\\nbt\\NbtStreamReader, 0)
ZEND_END_ARG_INFO()

#define INTEGER_TAG_METHODS(ClassName) \
static const zend_function_entry ClassName##_methods[] = { \
	PHP_ME(pocketmine_nbt_tag_##ClassName, __construct, arginfo_integerish_construct, ZEND_ACC_PUBLIC) \
	PHP_ME(pocketmine_nbt_tag_##ClassName, min, arginfo_integerish_get_value, ZEND_ACC_PROTECTED) \
	PHP_ME(pocketmine_nbt_tag_##ClassName, max, arginfo_integerish_get_value, ZEND_ACC_PROTECTED) \
	PHP_ME(pocketmine_nbt_tag_##ClassName, getTypeName, arginfo_tag_get_type_name, ZEND_ACC_PROTECTED) \
	PHP_ME(pocketmine_nbt_tag_##ClassName, getType, arginfo_tag_get_type, ZEND_ACC_PUBLIC) \
	PHP_ME(pocketmine_nbt_tag_##ClassName, read, arginfo_read_tag, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC) \
	PHP_ME(pocketmine_nbt_tag_##ClassName, write, arginfo_tag_write, ZEND_ACC_PUBLIC) \
	PHP_ME(pocketmine_nbt_tag_IntegerishTagTrait, getValue, arginfo_integerish_get_value, ZEND_ACC_PUBLIC) \
	PHP_ME(pocketmine_nbt_tag_IntegerishTagTrait, stringifyValue, arginfo_stringify_value, ZEND_ACC_PROTECTED) \
	PHP_FE_END \
};

INTEGER_TAG_METHODS(ByteTag)
INTEGER_TAG_METHODS(ShortTag)
INTEGER_TAG_METHODS(IntTag)
INTEGER_TAG_METHODS(LongTag)

// -------------------------------------------------------------
// FloatTag
// -------------------------------------------------------------
ZEND_BEGIN_ARG_INFO_EX(arginfo_float_construct, 0, 0, 1)
	ZEND_ARG_TYPE_INFO(0, value, IS_DOUBLE, 0)
ZEND_END_ARG_INFO()

PHP_METHOD(pocketmine_nbt_tag_FloatTag, __construct) {
	double value;
	ZEND_PARSE_PARAMETERS_START(1, -1)
		Z_PARAM_DOUBLE(value)
	ZEND_PARSE_PARAMETERS_END();
	check_arg_count("pocketmine\\nbt\\tag\\FloatTag::__construct", EX(func)->common.num_args, 1);
	zend_update_property_double(float_tag_ce, Z_OBJ_P(getThis()), "value", sizeof("value") - 1, value);
}

PHP_METHOD(pocketmine_nbt_tag_FloatTag, getValue) {
	ZEND_PARSE_PARAMETERS_NONE();
	zval rv;
	zval* val = zend_read_property(float_tag_ce, Z_OBJ_P(getThis()), "value", sizeof("value") - 1, 1, &rv);
	RETURN_ZVAL(val, 1, 0);
}

PHP_METHOD(pocketmine_nbt_tag_FloatTag, getType) {
	ZEND_PARSE_PARAMETERS_NONE();
	RETURN_LONG(NBT_TAG_FLOAT);
}

PHP_METHOD(pocketmine_nbt_tag_FloatTag, getTypeName) {
	ZEND_PARSE_PARAMETERS_NONE();
	RETURN_STRING("Float");
}

PHP_METHOD(pocketmine_nbt_tag_FloatTag, stringifyValue) {
	zend_long indentation;
	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_LONG(indentation)
	ZEND_PARSE_PARAMETERS_END();
	zval rv;
	zval* val = zend_read_property(float_tag_ce, Z_OBJ_P(getThis()), "value", sizeof("value") - 1, 1, &rv);
	convert_to_string(val);
	RETURN_ZVAL(val, 1, 0);
}

PHP_METHOD(pocketmine_nbt_tag_FloatTag, read) {
	zval* reader;
	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_OBJECT_OF_CLASS(reader, nbt_stream_reader_ce)
	ZEND_PARSE_PARAMETERS_END();
	zval val_zv;
	call_method_0(reader, "readFloat", &val_zv);
	object_init_ex(return_value, float_tag_ce);
	call_method_1(return_value, "__construct", NULL, &val_zv);
	zval_ptr_dtor(&val_zv);
}

PHP_METHOD(pocketmine_nbt_tag_FloatTag, write) {
	zval* writer;
	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_OBJECT_OF_CLASS(writer, nbt_stream_writer_ce)
	ZEND_PARSE_PARAMETERS_END();
	zval rv;
	zval* val = zend_read_property(float_tag_ce, Z_OBJ_P(getThis()), "value", sizeof("value") - 1, 1, &rv);
	zval dummy;
	call_method_1(writer, "writeFloat", &dummy, val);
	zval_ptr_dtor(&dummy);
}

PHP_METHOD(pocketmine_nbt_tag_FloatTag, equals) {
	zval* that;
	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_OBJECT_OF_CLASS(that, tag_ce)
	ZEND_PARSE_PARAMETERS_END();

	if (!instanceof_function(Z_OBJCE_P(that), float_tag_ce)) {
		RETURN_FALSE;
	}

	zval rv1, rv2;
	zval* v1 = zend_read_property(float_tag_ce, Z_OBJ_P(getThis()), "value", sizeof("value") - 1, 1, &rv1);
	zval* v2 = zend_read_property(float_tag_ce, Z_OBJ_P(that), "value", sizeof("value") - 1, 1, &rv2);

	float f1 = (float)zval_get_double(v1);
	float f2 = (float)zval_get_double(v2);

	uint32_t u1, u2;
	memcpy(&u1, &f1, sizeof(uint32_t));
	memcpy(&u2, &f2, sizeof(uint32_t));

	RETURN_BOOL(u1 == u2);
}

static const zend_function_entry float_tag_methods[] = {
	PHP_ME(pocketmine_nbt_tag_FloatTag, __construct, arginfo_float_construct, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_tag_FloatTag, getValue, arginfo_read_float, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_tag_FloatTag, getType, arginfo_tag_get_type, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_tag_FloatTag, getTypeName, arginfo_tag_get_type_name, ZEND_ACC_PROTECTED)
	PHP_ME(pocketmine_nbt_tag_FloatTag, stringifyValue, arginfo_stringify_value, ZEND_ACC_PROTECTED)
	PHP_ME(pocketmine_nbt_tag_FloatTag, read, arginfo_read_tag, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	PHP_ME(pocketmine_nbt_tag_FloatTag, write, arginfo_tag_write, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_tag_FloatTag, equals, arginfo_tag_equals, ZEND_ACC_PUBLIC)
	PHP_FE_END
};

// -------------------------------------------------------------
// DoubleTag
// -------------------------------------------------------------
PHP_METHOD(pocketmine_nbt_tag_DoubleTag, __construct) {
	double value;
	ZEND_PARSE_PARAMETERS_START(1, -1)
		Z_PARAM_DOUBLE(value)
	ZEND_PARSE_PARAMETERS_END();
	check_arg_count("pocketmine\\nbt\\tag\\DoubleTag::__construct", EX(func)->common.num_args, 1);
	zend_update_property_double(double_tag_ce, Z_OBJ_P(getThis()), "value", sizeof("value") - 1, value);
}

PHP_METHOD(pocketmine_nbt_tag_DoubleTag, getValue) {
	ZEND_PARSE_PARAMETERS_NONE();
	zval rv;
	zval* val = zend_read_property(double_tag_ce, Z_OBJ_P(getThis()), "value", sizeof("value") - 1, 1, &rv);
	RETURN_ZVAL(val, 1, 0);
}

PHP_METHOD(pocketmine_nbt_tag_DoubleTag, getType) {
	ZEND_PARSE_PARAMETERS_NONE();
	RETURN_LONG(NBT_TAG_DOUBLE);
}

PHP_METHOD(pocketmine_nbt_tag_DoubleTag, getTypeName) {
	ZEND_PARSE_PARAMETERS_NONE();
	RETURN_STRING("Double");
}

PHP_METHOD(pocketmine_nbt_tag_DoubleTag, stringifyValue) {
	zend_long indentation;
	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_LONG(indentation)
	ZEND_PARSE_PARAMETERS_END();
	zval rv;
	zval* val = zend_read_property(double_tag_ce, Z_OBJ_P(getThis()), "value", sizeof("value") - 1, 1, &rv);
	convert_to_string(val);
	RETURN_ZVAL(val, 1, 0);
}

PHP_METHOD(pocketmine_nbt_tag_DoubleTag, read) {
	zval* reader;
	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_OBJECT_OF_CLASS(reader, nbt_stream_reader_ce)
	ZEND_PARSE_PARAMETERS_END();
	zval val_zv;
	call_method_0(reader, "readDouble", &val_zv);
	object_init_ex(return_value, double_tag_ce);
	call_method_1(return_value, "__construct", NULL, &val_zv);
	zval_ptr_dtor(&val_zv);
}

PHP_METHOD(pocketmine_nbt_tag_DoubleTag, write) {
	zval* writer;
	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_OBJECT_OF_CLASS(writer, nbt_stream_writer_ce)
	ZEND_PARSE_PARAMETERS_END();
	zval rv;
	zval* val = zend_read_property(double_tag_ce, Z_OBJ_P(getThis()), "value", sizeof("value") - 1, 1, &rv);
	zval dummy;
	call_method_1(writer, "writeDouble", &dummy, val);
	zval_ptr_dtor(&dummy);
}

static const zend_function_entry double_tag_methods[] = {
	PHP_ME(pocketmine_nbt_tag_DoubleTag, __construct, arginfo_float_construct, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_tag_DoubleTag, getValue, arginfo_read_float, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_tag_DoubleTag, getType, arginfo_tag_get_type, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_tag_DoubleTag, getTypeName, arginfo_tag_get_type_name, ZEND_ACC_PROTECTED)
	PHP_ME(pocketmine_nbt_tag_DoubleTag, stringifyValue, arginfo_stringify_value, ZEND_ACC_PROTECTED)
	PHP_ME(pocketmine_nbt_tag_DoubleTag, read, arginfo_read_tag, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	PHP_ME(pocketmine_nbt_tag_DoubleTag, write, arginfo_tag_write, ZEND_ACC_PUBLIC)
	PHP_FE_END
};

// -------------------------------------------------------------
// ByteArrayTag
// -------------------------------------------------------------
ZEND_BEGIN_ARG_INFO_EX(arginfo_string_construct, 0, 0, 1)
	ZEND_ARG_TYPE_INFO(0, value, IS_STRING, 0)
ZEND_END_ARG_INFO()

PHP_METHOD(pocketmine_nbt_tag_ByteArrayTag, __construct) {
	zend_string* value;
	ZEND_PARSE_PARAMETERS_START(1, -1)
		Z_PARAM_STR(value)
	ZEND_PARSE_PARAMETERS_END();
	check_arg_count("pocketmine\\nbt\\tag\\ByteArrayTag::__construct", EX(func)->common.num_args, 1);
	zend_update_property_str(byte_array_tag_ce, Z_OBJ_P(getThis()), "value", sizeof("value") - 1, value);
}

PHP_METHOD(pocketmine_nbt_tag_ByteArrayTag, getValue) {
	ZEND_PARSE_PARAMETERS_NONE();
	zval rv;
	zval* val = zend_read_property(byte_array_tag_ce, Z_OBJ_P(getThis()), "value", sizeof("value") - 1, 1, &rv);
	RETURN_ZVAL(val, 1, 0);
}

PHP_METHOD(pocketmine_nbt_tag_ByteArrayTag, getType) {
	ZEND_PARSE_PARAMETERS_NONE();
	RETURN_LONG(NBT_TAG_BYTE_ARRAY);
}

PHP_METHOD(pocketmine_nbt_tag_ByteArrayTag, getTypeName) {
	ZEND_PARSE_PARAMETERS_NONE();
	RETURN_STRING("ByteArray");
}

PHP_METHOD(pocketmine_nbt_tag_ByteArrayTag, stringifyValue) {
	zend_long indentation;
	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_LONG(indentation)
	ZEND_PARSE_PARAMETERS_END();
	zval rv;
	zval* val = zend_read_property(byte_array_tag_ce, Z_OBJ_P(getThis()), "value", sizeof("value") - 1, 1, &rv);
	zend_string* b64 = php_base64_encode((const unsigned char*)Z_STRVAL_P(val), Z_STRLEN_P(val));
	smart_str res = {0};
	smart_str_appends(&res, "b64:");
	smart_str_append(&res, b64);
	zend_string_release(b64);
	smart_str_0(&res);
	RETURN_STR(res.s);
}

PHP_METHOD(pocketmine_nbt_tag_ByteArrayTag, read) {
	zval* reader;
	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_OBJECT_OF_CLASS(reader, nbt_stream_reader_ce)
	ZEND_PARSE_PARAMETERS_END();
	zval val_zv;
	call_method_0(reader, "readByteArray", &val_zv);
	object_init_ex(return_value, byte_array_tag_ce);
	call_method_1(return_value, "__construct", NULL, &val_zv);
	zval_ptr_dtor(&val_zv);
}

PHP_METHOD(pocketmine_nbt_tag_ByteArrayTag, write) {
	zval* writer;
	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_OBJECT_OF_CLASS(writer, nbt_stream_writer_ce)
	ZEND_PARSE_PARAMETERS_END();
	zval rv;
	zval* val = zend_read_property(byte_array_tag_ce, Z_OBJ_P(getThis()), "value", sizeof("value") - 1, 1, &rv);
	zval dummy;
	call_method_1(writer, "writeByteArray", &dummy, val);
	zval_ptr_dtor(&dummy);
}

static const zend_function_entry byte_array_tag_methods[] = {
	PHP_ME(pocketmine_nbt_tag_ByteArrayTag, __construct, arginfo_string_construct, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_tag_ByteArrayTag, getValue, arginfo_read_string, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_tag_ByteArrayTag, getType, arginfo_tag_get_type, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_tag_ByteArrayTag, getTypeName, arginfo_tag_get_type_name, ZEND_ACC_PROTECTED)
	PHP_ME(pocketmine_nbt_tag_ByteArrayTag, stringifyValue, arginfo_stringify_value, ZEND_ACC_PROTECTED)
	PHP_ME(pocketmine_nbt_tag_ByteArrayTag, read, arginfo_read_tag, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	PHP_ME(pocketmine_nbt_tag_ByteArrayTag, write, arginfo_tag_write, ZEND_ACC_PUBLIC)
	PHP_FE_END
};

// -------------------------------------------------------------
// StringTag
// -------------------------------------------------------------
PHP_METHOD(pocketmine_nbt_tag_StringTag, __construct) {
	zend_string* value;
	ZEND_PARSE_PARAMETERS_START(1, -1)
		Z_PARAM_STR(value)
	ZEND_PARSE_PARAMETERS_END();
	check_arg_count("pocketmine\\nbt\\tag\\StringTag::__construct", EX(func)->common.num_args, 1);
	if (ZSTR_LEN(value) > 32767) {
		throw_invalid_tag_value_exception("StringTag cannot hold more than 32767 bytes, got string of length %zd", ZSTR_LEN(value));
		RETURN_THROWS();
	}
	zend_update_property_str(string_tag_ce, Z_OBJ_P(getThis()), "value", sizeof("value") - 1, value);
}

PHP_METHOD(pocketmine_nbt_tag_StringTag, getValue) {
	ZEND_PARSE_PARAMETERS_NONE();
	zval rv;
	zval* val = zend_read_property(string_tag_ce, Z_OBJ_P(getThis()), "value", sizeof("value") - 1, 1, &rv);
	RETURN_ZVAL(val, 1, 0);
}

PHP_METHOD(pocketmine_nbt_tag_StringTag, getType) {
	ZEND_PARSE_PARAMETERS_NONE();
	RETURN_LONG(NBT_TAG_STRING);
}

PHP_METHOD(pocketmine_nbt_tag_StringTag, getTypeName) {
	ZEND_PARSE_PARAMETERS_NONE();
	RETURN_STRING("String");
}

PHP_METHOD(pocketmine_nbt_tag_StringTag, stringifyValue) {
	zend_long indentation;
	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_LONG(indentation)
	ZEND_PARSE_PARAMETERS_END();
	zval rv;
	zval* val = zend_read_property(string_tag_ce, Z_OBJ_P(getThis()), "value", sizeof("value") - 1, 1, &rv);
	smart_str res = {0};
	smart_str_appendc(&res, '"');
	smart_str_appendl(&res, Z_STRVAL_P(val), Z_STRLEN_P(val));
	smart_str_appendc(&res, '"');
	smart_str_0(&res);
	RETURN_STR(res.s);
}

PHP_METHOD(pocketmine_nbt_tag_StringTag, read) {
	zval* reader;
	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_OBJECT_OF_CLASS(reader, nbt_stream_reader_ce)
	ZEND_PARSE_PARAMETERS_END();
	zval val_zv;
	call_method_0(reader, "readString", &val_zv);
	object_init_ex(return_value, string_tag_ce);
	call_method_1(return_value, "__construct", NULL, &val_zv);
	zval_ptr_dtor(&val_zv);
}

PHP_METHOD(pocketmine_nbt_tag_StringTag, write) {
	zval* writer;
	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_OBJECT_OF_CLASS(writer, nbt_stream_writer_ce)
	ZEND_PARSE_PARAMETERS_END();
	zval rv;
	zval* val = zend_read_property(string_tag_ce, Z_OBJ_P(getThis()), "value", sizeof("value") - 1, 1, &rv);
	zval dummy;
	call_method_1(writer, "writeString", &dummy, val);
	zval_ptr_dtor(&dummy);
}

static const zend_function_entry string_tag_methods[] = {
	PHP_ME(pocketmine_nbt_tag_StringTag, __construct, arginfo_string_construct, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_tag_StringTag, getValue, arginfo_read_string, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_tag_StringTag, getType, arginfo_tag_get_type, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_tag_StringTag, getTypeName, arginfo_tag_get_type_name, ZEND_ACC_PROTECTED)
	PHP_ME(pocketmine_nbt_tag_StringTag, stringifyValue, arginfo_stringify_value, ZEND_ACC_PROTECTED)
	PHP_ME(pocketmine_nbt_tag_StringTag, read, arginfo_read_tag, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	PHP_ME(pocketmine_nbt_tag_StringTag, write, arginfo_tag_write, ZEND_ACC_PUBLIC)
	PHP_FE_END
};

// -------------------------------------------------------------
// IntArrayTag
// -------------------------------------------------------------
PHP_METHOD(pocketmine_nbt_tag_IntArrayTag, __construct) {
	zval* value;
	ZEND_PARSE_PARAMETERS_START(1, -1)
		Z_PARAM_ARRAY(value)
	ZEND_PARSE_PARAMETERS_END();
	check_arg_count("pocketmine\\nbt\\tag\\IntArrayTag::__construct", EX(func)->common.num_args, 1);

	zval reindexed;
	array_init_size(&reindexed, zend_hash_num_elements(Z_ARRVAL_P(value)));

	zval* entry;
	ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(value), entry) {
		if (Z_TYPE_P(entry) != IS_LONG) {
			zend_throw_error(zend_ce_type_error, "Array element must be an integer");
			zval_ptr_dtor(&reindexed);
			RETURN_THROWS();
		}
		add_next_index_long(&reindexed, Z_LVAL_P(entry));
	} ZEND_HASH_FOREACH_END();

	zend_update_property(int_array_tag_ce, Z_OBJ_P(getThis()), "value", sizeof("value") - 1, &reindexed);
	zval_ptr_dtor(&reindexed);
}

PHP_METHOD(pocketmine_nbt_tag_IntArrayTag, getValue) {
	ZEND_PARSE_PARAMETERS_NONE();
	zval rv;
	zval* val = zend_read_property(int_array_tag_ce, Z_OBJ_P(getThis()), "value", sizeof("value") - 1, 1, &rv);
	RETURN_ZVAL(val, 1, 0);
}

PHP_METHOD(pocketmine_nbt_tag_IntArrayTag, getType) {
	ZEND_PARSE_PARAMETERS_NONE();
	RETURN_LONG(NBT_TAG_INT_ARRAY);
}

PHP_METHOD(pocketmine_nbt_tag_IntArrayTag, getTypeName) {
	ZEND_PARSE_PARAMETERS_NONE();
	RETURN_STRING("IntArray");
}

PHP_METHOD(pocketmine_nbt_tag_IntArrayTag, stringifyValue) {
	zend_long indentation;
	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_LONG(indentation)
	ZEND_PARSE_PARAMETERS_END();

	zval rv;
	zval* val = zend_read_property(int_array_tag_ce, Z_OBJ_P(getThis()), "value", sizeof("value") - 1, 1, &rv);

	smart_str output = {0};
	smart_str_appendc(&output, '[');
	zend_bool first = 1;
	zval* entry;
	ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(val), entry) {
		if (!first) smart_str_appendc(&output, ',');
		smart_str_append_long(&output, zval_get_long(entry));
		first = 0;
	} ZEND_HASH_FOREACH_END();
	smart_str_appendc(&output, ']');
	smart_str_0(&output);
	RETURN_STR(output.s);
}

PHP_METHOD(pocketmine_nbt_tag_IntArrayTag, read) {
	zval* reader;
	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_OBJECT_OF_CLASS(reader, nbt_stream_reader_ce)
	ZEND_PARSE_PARAMETERS_END();
	zval val_zv;
	call_method_0(reader, "readIntArray", &val_zv);
	object_init_ex(return_value, int_array_tag_ce);
	call_method_1(return_value, "__construct", NULL, &val_zv);
	zval_ptr_dtor(&val_zv);
}

PHP_METHOD(pocketmine_nbt_tag_IntArrayTag, write) {
	zval* writer;
	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_OBJECT_OF_CLASS(writer, nbt_stream_writer_ce)
	ZEND_PARSE_PARAMETERS_END();
	zval rv;
	zval* val = zend_read_property(int_array_tag_ce, Z_OBJ_P(getThis()), "value", sizeof("value") - 1, 1, &rv);
	zval dummy;
	call_method_1(writer, "writeIntArray", &dummy, val);
	zval_ptr_dtor(&dummy);
}

ZEND_BEGIN_ARG_INFO_EX(arginfo_int_array_construct, 0, 0, 1)
	ZEND_ARG_TYPE_INFO(0, value, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

static const zend_function_entry int_array_tag_methods[] = {
	PHP_ME(pocketmine_nbt_tag_IntArrayTag, __construct, arginfo_int_array_construct, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_tag_IntArrayTag, getValue, arginfo_read_array, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_tag_IntArrayTag, getType, arginfo_tag_get_type, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_tag_IntArrayTag, getTypeName, arginfo_tag_get_type_name, ZEND_ACC_PROTECTED)
	PHP_ME(pocketmine_nbt_tag_IntArrayTag, stringifyValue, arginfo_stringify_value, ZEND_ACC_PROTECTED)
	PHP_ME(pocketmine_nbt_tag_IntArrayTag, read, arginfo_read_tag, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	PHP_ME(pocketmine_nbt_tag_IntArrayTag, write, arginfo_tag_write, ZEND_ACC_PUBLIC)
	PHP_FE_END
};

// -------------------------------------------------------------
// ListTag
// -------------------------------------------------------------
ZEND_BEGIN_ARG_INFO_EX(arginfo_list_construct, 0, 0, 0)
	ZEND_ARG_TYPE_INFO(0, value, IS_ARRAY, 0)
	ZEND_ARG_TYPE_INFO(0, tagType, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_list_push, 0, 1, IS_VOID, 0)
	ZEND_ARG_OBJ_INFO(0, tag, pocketmine\\nbt\\tag\\Tag, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_list_pop, 0, 0, pocketmine\\nbt\\tag\\Tag, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_list_insert, 0, 2, IS_VOID, 0)
	ZEND_ARG_TYPE_INFO(0, offset, IS_LONG, 0)
	ZEND_ARG_OBJ_INFO(0, tag, pocketmine\\nbt\\tag\\Tag, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_list_remove, 0, 1, IS_VOID, 0)
	ZEND_ARG_TYPE_INFO(0, offset, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_list_get, 0, 1, pocketmine\\nbt\\tag\\Tag, 0)
	ZEND_ARG_TYPE_INFO(0, offset, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_list_isset, 0, 1, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, offset, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_list_empty, 0, 0, _IS_BOOL, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_list_set_tag_type, 0, 1, IS_VOID, 0)
	ZEND_ARG_TYPE_INFO(0, type, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_list_cast, 0, 1, pocketmine\\nbt\\tag\\ListTag, 1)
	ZEND_ARG_TYPE_INFO(0, tagClass, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_list_read, 0, 0, 2)
	ZEND_ARG_OBJ_INFO(0, reader, pocketmine\\nbt\\NbtStreamReader, 0)
	ZEND_ARG_OBJ_INFO(0, tracker, pocketmine\\nbt\\ReaderTracker, 0)
ZEND_END_ARG_INFO()

static void check_list_tag_type(zval* list_obj, zval* tag_obj) {
	zval rv_type, rv_val;
	zval* tagType_zv = zend_read_property(list_tag_ce, Z_OBJ_P(list_obj), "tagType", sizeof("tagType") - 1, 1, &rv_type);
	zend_long tagType = zval_get_long(tagType_zv);

	zval type_zv;
	call_method_0(tag_obj, "getType", &type_zv);

	zend_long tag_type = zval_get_long(&type_zv);

	zval* val_zv = zend_read_property(list_tag_ce, Z_OBJ_P(list_obj), "value", sizeof("value") - 1, 1, &rv_val);
	uint32_t count = zend_hash_num_elements(Z_ARRVAL_P(val_zv));

	if (tag_type != tagType) {
		if (count == 0) {
			zend_update_property_long(list_tag_ce, Z_OBJ_P(list_obj), "tagType", sizeof("tagType") - 1, tag_type);
		} else {
			zval* first_entry = zend_hash_index_find(Z_ARRVAL_P(val_zv), 0);
			zend_throw_error(zend_ce_type_error,
				"Invalid tag of type %s assigned to ListTag, expected %s",
				ZSTR_VAL(Z_OBJCE_P(tag_obj)->name),
				first_entry ? ZSTR_VAL(Z_OBJCE_P(first_entry)->name) : "unknown");
		}
	}
}

PHP_METHOD(pocketmine_nbt_tag_ListTag, __construct) {
	zval* value = NULL;
	zend_long tagType = NBT_TAG_END;
	ZEND_PARSE_PARAMETERS_START(0, -1)
		Z_PARAM_OPTIONAL
		Z_PARAM_ARRAY(value)
		Z_PARAM_LONG(tagType)
	ZEND_PARSE_PARAMETERS_END();
	check_arg_count("pocketmine\\nbt\\tag\\ListTag::__construct", EX(func)->common.num_args, 2);

	zend_update_property_long(list_tag_ce, Z_OBJ_P(getThis()), "tagType", sizeof("tagType") - 1, tagType);

	zval empty_arr;
	array_init_size(&empty_arr, value ? zend_hash_num_elements(Z_ARRVAL_P(value)) : 0);
	zend_update_property(list_tag_ce, Z_OBJ_P(getThis()), "value", sizeof("value") - 1, &empty_arr);
	zval_ptr_dtor(&empty_arr);

	if (value && Z_TYPE_P(value) == IS_ARRAY) {
		zval* entry;
		ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(value), entry) {
			zval dummy;
			call_method_1(getThis(), "push", &dummy, entry);
			zval_ptr_dtor(&dummy);
			if (EG(exception)) return;
		} ZEND_HASH_FOREACH_END();
	}
}

PHP_METHOD(pocketmine_nbt_tag_ListTag, getValue) {
	ZEND_PARSE_PARAMETERS_NONE();
	zval rv;
	zval* val = zend_read_property(list_tag_ce, Z_OBJ_P(getThis()), "value", sizeof("value") - 1, 1, &rv);
	RETURN_ZVAL(val, 1, 0);
}

PHP_METHOD(pocketmine_nbt_tag_ListTag, getAllValues) {
	ZEND_PARSE_PARAMETERS_NONE();
	zval rv;
	zval* val = zend_read_property(list_tag_ce, Z_OBJ_P(getThis()), "value", sizeof("value") - 1, 1, &rv);
	array_init_size(return_value, zend_hash_num_elements(Z_ARRVAL_P(val)));

	zval* entry;
	ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(val), entry) {
		zval item_val;
		call_method_0(entry, "getValue", &item_val);
		add_next_index_zval(return_value, &item_val);
	} ZEND_HASH_FOREACH_END();
}

PHP_METHOD(pocketmine_nbt_tag_ListTag, count) {
	ZEND_PARSE_PARAMETERS_NONE();
	zval rv;
	zval* val = zend_read_property(list_tag_ce, Z_OBJ_P(getThis()), "value", sizeof("value") - 1, 1, &rv);
	RETURN_LONG(zend_hash_num_elements(Z_ARRVAL_P(val)));
}

PHP_METHOD(pocketmine_nbt_tag_ListTag, getCount) {
	ZEND_PARSE_PARAMETERS_NONE();
	zval rv;
	zval* val = zend_read_property(list_tag_ce, Z_OBJ_P(getThis()), "value", sizeof("value") - 1, 1, &rv);
	RETURN_LONG(zend_hash_num_elements(Z_ARRVAL_P(val)));
}

PHP_METHOD(pocketmine_nbt_tag_ListTag, push) {
	zval* tag;
	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_OBJECT_OF_CLASS(tag, tag_ce)
	ZEND_PARSE_PARAMETERS_END();

	check_list_tag_type(getThis(), tag);
	if (EG(exception)) return;

	zval rv;
	zval* val = zend_read_property(list_tag_ce, Z_OBJ_P(getThis()), "value", sizeof("value") - 1, 1, &rv);
	Z_TRY_ADDREF_P(tag);
	add_next_index_zval(val, tag);
}

PHP_METHOD(pocketmine_nbt_tag_ListTag, pop) {
	ZEND_PARSE_PARAMETERS_NONE();
	zval rv;
	zval* val = zend_read_property(list_tag_ce, Z_OBJ_P(getThis()), "value", sizeof("value") - 1, 1, &rv);
	uint32_t count = zend_hash_num_elements(Z_ARRVAL_P(val));
	if (count == 0) {
		zend_throw_exception(spl_ce_LogicException, "List is empty", 0);
		RETURN_THROWS();
	}
	zval* last = zend_hash_index_find(Z_ARRVAL_P(val), count - 1);
	ZVAL_COPY(return_value, last);
	zend_hash_index_del(Z_ARRVAL_P(val), count - 1);
}

PHP_METHOD(pocketmine_nbt_tag_ListTag, shift) {
	ZEND_PARSE_PARAMETERS_NONE();
	zval rv;
	zval* val = zend_read_property(list_tag_ce, Z_OBJ_P(getThis()), "value", sizeof("value") - 1, 1, &rv);
	uint32_t count = zend_hash_num_elements(Z_ARRVAL_P(val));
	if (count == 0) {
		zend_throw_exception(spl_ce_LogicException, "List is empty", 0);
		RETURN_THROWS();
	}

	zval* first = zend_hash_index_find(Z_ARRVAL_P(val), 0);
	ZVAL_COPY(return_value, first);

	// Reindex
	zval new_arr;
	array_init_size(&new_arr, count - 1);
	for (uint32_t i = 1; i < count; ++i) {
		zval* item = zend_hash_index_find(Z_ARRVAL_P(val), i);
		Z_TRY_ADDREF_P(item);
		add_next_index_zval(&new_arr, item);
	}
	zend_update_property(list_tag_ce, Z_OBJ_P(getThis()), "value", sizeof("value") - 1, &new_arr);
	zval_ptr_dtor(&new_arr);
}

PHP_METHOD(pocketmine_nbt_tag_ListTag, unshift) {
	zval* tag;
	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_OBJECT_OF_CLASS(tag, tag_ce)
	ZEND_PARSE_PARAMETERS_END();

	check_list_tag_type(getThis(), tag);
	if (EG(exception)) return;

	zval rv;
	zval* val = zend_read_property(list_tag_ce, Z_OBJ_P(getThis()), "value", sizeof("value") - 1, 1, &rv);
	uint32_t count = zend_hash_num_elements(Z_ARRVAL_P(val));

	zval new_arr;
	array_init_size(&new_arr, count + 1);
	Z_TRY_ADDREF_P(tag);
	add_next_index_zval(&new_arr, tag);

	for (uint32_t i = 0; i < count; ++i) {
		zval* item = zend_hash_index_find(Z_ARRVAL_P(val), i);
		Z_TRY_ADDREF_P(item);
		add_next_index_zval(&new_arr, item);
	}
	zend_update_property(list_tag_ce, Z_OBJ_P(getThis()), "value", sizeof("value") - 1, &new_arr);
	zval_ptr_dtor(&new_arr);
}

PHP_METHOD(pocketmine_nbt_tag_ListTag, insert) {
	zend_long offset;
	zval* tag;
	ZEND_PARSE_PARAMETERS_START(2, 2)
		Z_PARAM_LONG(offset)
		Z_PARAM_OBJECT_OF_CLASS(tag, tag_ce)
	ZEND_PARSE_PARAMETERS_END();

	check_list_tag_type(getThis(), tag);
	if (EG(exception)) return;

	zval rv;
	zval* val = zend_read_property(list_tag_ce, Z_OBJ_P(getThis()), "value", sizeof("value") - 1, 1, &rv);
	zend_long count = zend_hash_num_elements(Z_ARRVAL_P(val));

	if (offset < 0 || offset > count) {
		zend_throw_exception(spl_ce_OutOfRangeException, "Offset cannot be negative or larger than the list's current size", 0);
		RETURN_THROWS();
	}

	zval new_arr;
	array_init_size(&new_arr, count + 1);
	for (zend_long i = 0; i < offset; ++i) {
		zval* item = zend_hash_index_find(Z_ARRVAL_P(val), i);
		Z_TRY_ADDREF_P(item);
		add_next_index_zval(&new_arr, item);
	}
	Z_TRY_ADDREF_P(tag);
	add_next_index_zval(&new_arr, tag);
	for (zend_long i = offset; i < count; ++i) {
		zval* item = zend_hash_index_find(Z_ARRVAL_P(val), i);
		Z_TRY_ADDREF_P(item);
		add_next_index_zval(&new_arr, item);
	}
	zend_update_property(list_tag_ce, Z_OBJ_P(getThis()), "value", sizeof("value") - 1, &new_arr);
	zval_ptr_dtor(&new_arr);
}

PHP_METHOD(pocketmine_nbt_tag_ListTag, remove) {
	zend_long offset;
	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_LONG(offset)
	ZEND_PARSE_PARAMETERS_END();

	zval rv;
	zval* val = zend_read_property(list_tag_ce, Z_OBJ_P(getThis()), "value", sizeof("value") - 1, 1, &rv);
	zend_long count = zend_hash_num_elements(Z_ARRVAL_P(val));

	if (offset >= 0 && offset < count) {
		zval new_arr;
		array_init_size(&new_arr, count - 1);
		for (zend_long i = 0; i < count; ++i) {
			if (i == offset) continue;
			zval* item = zend_hash_index_find(Z_ARRVAL_P(val), i);
			Z_TRY_ADDREF_P(item);
			add_next_index_zval(&new_arr, item);
		}
		zend_update_property(list_tag_ce, Z_OBJ_P(getThis()), "value", sizeof("value") - 1, &new_arr);
		zval_ptr_dtor(&new_arr);
	}
}

PHP_METHOD(pocketmine_nbt_tag_ListTag, get) {
	zend_long offset;
	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_LONG(offset)
	ZEND_PARSE_PARAMETERS_END();

	zval rv;
	zval* val = zend_read_property(list_tag_ce, Z_OBJ_P(getThis()), "value", sizeof("value") - 1, 1, &rv);
	zval* item = zend_hash_index_find(Z_ARRVAL_P(val), offset);
	if (!item) {
		zend_throw_exception_ex(spl_ce_OutOfRangeException, 0, "No such tag at offset %ld", (long)offset);
		RETURN_THROWS();
	}
	RETURN_ZVAL(item, 1, 0);
}

PHP_METHOD(pocketmine_nbt_tag_ListTag, first) {
	ZEND_PARSE_PARAMETERS_NONE();
	zval rv;
	zval* val = zend_read_property(list_tag_ce, Z_OBJ_P(getThis()), "value", sizeof("value") - 1, 1, &rv);
	zval* item = zend_hash_index_find(Z_ARRVAL_P(val), 0);
	if (!item) {
		zend_throw_exception(spl_ce_LogicException, "List is empty", 0);
		RETURN_THROWS();
	}
	RETURN_ZVAL(item, 1, 0);
}

PHP_METHOD(pocketmine_nbt_tag_ListTag, last) {
	ZEND_PARSE_PARAMETERS_NONE();
	zval rv;
	zval* val = zend_read_property(list_tag_ce, Z_OBJ_P(getThis()), "value", sizeof("value") - 1, 1, &rv);
	uint32_t count = zend_hash_num_elements(Z_ARRVAL_P(val));
	if (count == 0) {
		zend_throw_exception(spl_ce_LogicException, "List is empty", 0);
		RETURN_THROWS();
	}
	zval* item = zend_hash_index_find(Z_ARRVAL_P(val), count - 1);
	RETURN_ZVAL(item, 1, 0);
}

PHP_METHOD(pocketmine_nbt_tag_ListTag, set) {
	zend_long offset;
	zval* tag;
	ZEND_PARSE_PARAMETERS_START(2, 2)
		Z_PARAM_LONG(offset)
		Z_PARAM_OBJECT_OF_CLASS(tag, tag_ce)
	ZEND_PARSE_PARAMETERS_END();

	check_list_tag_type(getThis(), tag);
	if (EG(exception)) return;

	zval rv;
	zval* val = zend_read_property(list_tag_ce, Z_OBJ_P(getThis()), "value", sizeof("value") - 1, 1, &rv);
	zend_long count = zend_hash_num_elements(Z_ARRVAL_P(val));

	if (offset < 0 || offset > count) {
		zend_throw_exception(spl_ce_OutOfRangeException, "Offset cannot be negative or larger than the list's current size", 0);
		RETURN_THROWS();
	}

	Z_TRY_ADDREF_P(tag);
	zend_hash_index_update(Z_ARRVAL_P(val), offset, tag);
}

PHP_METHOD(pocketmine_nbt_tag_ListTag, isset) {
	zend_long offset;
	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_LONG(offset)
	ZEND_PARSE_PARAMETERS_END();

	zval rv;
	zval* val = zend_read_property(list_tag_ce, Z_OBJ_P(getThis()), "value", sizeof("value") - 1, 1, &rv);
	RETURN_BOOL(zend_hash_index_exists(Z_ARRVAL_P(val), offset));
}

PHP_METHOD(pocketmine_nbt_tag_ListTag, empty) {
	ZEND_PARSE_PARAMETERS_NONE();
	zval rv;
	zval* val = zend_read_property(list_tag_ce, Z_OBJ_P(getThis()), "value", sizeof("value") - 1, 1, &rv);
	RETURN_BOOL(zend_hash_num_elements(Z_ARRVAL_P(val)) == 0);
}

PHP_METHOD(pocketmine_nbt_tag_ListTag, getType) {
	ZEND_PARSE_PARAMETERS_NONE();
	RETURN_LONG(NBT_TAG_LIST);
}

PHP_METHOD(pocketmine_nbt_tag_ListTag, getTypeName) {
	ZEND_PARSE_PARAMETERS_NONE();
	RETURN_STRING("List");
}

PHP_METHOD(pocketmine_nbt_tag_ListTag, getTagType) {
	ZEND_PARSE_PARAMETERS_NONE();
	zval rv;
	zval* val = zend_read_property(list_tag_ce, Z_OBJ_P(getThis()), "tagType", sizeof("tagType") - 1, 1, &rv);
	RETURN_ZVAL(val, 1, 0);
}

PHP_METHOD(pocketmine_nbt_tag_ListTag, setTagType) {
	zend_long type;
	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_LONG(type)
	ZEND_PARSE_PARAMETERS_END();

	zval rv;
	zval* val = zend_read_property(list_tag_ce, Z_OBJ_P(getThis()), "value", sizeof("value") - 1, 1, &rv);
	if (zend_hash_num_elements(Z_ARRVAL_P(val)) > 0) {
		zend_throw_exception(spl_ce_LogicException, "Cannot change tag type of non-empty ListTag", 0);
		RETURN_THROWS();
	}

	zend_update_property_long(list_tag_ce, Z_OBJ_P(getThis()), "tagType", sizeof("tagType") - 1, type);
}

PHP_METHOD(pocketmine_nbt_tag_ListTag, cast) {
	zend_string* tagClass;
	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_STR(tagClass)
	ZEND_PARSE_PARAMETERS_END();

	zval rv;
	zval* val = zend_read_property(list_tag_ce, Z_OBJ_P(getThis()), "value", sizeof("value") - 1, 1, &rv);
	if (zend_hash_num_elements(Z_ARRVAL_P(val)) == 0) {
		RETURN_ZVAL(getThis(), 1, 0);
	}

	zval* first = zend_hash_index_find(Z_ARRVAL_P(val), 0);
	zend_class_entry* target_ce = zend_lookup_class(tagClass);
	if (target_ce && first && instanceof_function(Z_OBJCE_P(first), target_ce)) {
		RETURN_ZVAL(getThis(), 1, 0);
	}

	RETURN_NULL();
}

PHP_METHOD(pocketmine_nbt_tag_ListTag, write) {
	zval* writer;
	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_OBJECT_OF_CLASS(writer, nbt_stream_writer_ce)
	ZEND_PARSE_PARAMETERS_END();

	zval rv_type, rv_val;
	zval* tagType = zend_read_property(list_tag_ce, Z_OBJ_P(getThis()), "tagType", sizeof("tagType") - 1, 1, &rv_type);
	zval* val = zend_read_property(list_tag_ce, Z_OBJ_P(getThis()), "value", sizeof("value") - 1, 1, &rv_val);

	zval dummy;
	call_method_1(writer, "writeByte", &dummy, tagType);
	zval_ptr_dtor(&dummy);

	zval count_zv;
	ZVAL_LONG(&count_zv, zend_hash_num_elements(Z_ARRVAL_P(val)));
	call_method_1(writer, "writeInt", &dummy, &count_zv);
	zval_ptr_dtor(&dummy);

	zval* entry;
	ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(val), entry) {
		call_method_1(entry, "write", &dummy, writer);
		zval_ptr_dtor(&dummy);
	} ZEND_HASH_FOREACH_END();
}

PHP_METHOD(pocketmine_nbt_tag_ListTag, stringifyValue) {
	zend_long indentation;
	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_LONG(indentation)
	ZEND_PARSE_PARAMETERS_END();

	zval rv;
	zval* val = zend_read_property(list_tag_ce, Z_OBJ_P(getThis()), "value", sizeof("value") - 1, 1, &rv);

	smart_str output = {0};
	smart_str_appends(&output, "{\n");

	zval* entry;
	ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(val), entry) {
		zval str_zv;
		zval arg;
		ZVAL_LONG(&arg, indentation + 1);
		call_method_1(entry, "toString", &str_zv, &arg);
		for (zend_long i = 0; i < (indentation + 1) * 2; ++i) smart_str_appendc(&output, ' ');
		smart_str_appendl(&output, Z_STRVAL(str_zv), Z_STRLEN(str_zv));
		smart_str_appendc(&output, '\n');
		zval_ptr_dtor(&str_zv);
	} ZEND_HASH_FOREACH_END();

	for (zend_long i = 0; i < indentation * 2; ++i) smart_str_appendc(&output, ' ');
	smart_str_appendc(&output, '}');
	smart_str_0(&output);
	RETURN_STR(output.s);
}

PHP_METHOD(pocketmine_nbt_tag_ListTag, __clone) {
	zval rv;
	zval* val = zend_read_property(list_tag_ce, Z_OBJ_P(getThis()), "value", sizeof("value") - 1, 1, &rv);
	uint32_t count = zend_hash_num_elements(Z_ARRVAL_P(val));

	zval new_val;
	array_init_size(&new_val, count);

	zval* entry;
	ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(val), entry) {
		zval cloned_child;
		call_method_0(entry, "safeClone", &cloned_child);
		if (EG(exception)) {
			zval_ptr_dtor(&new_val);
			return;
		}
		add_next_index_zval(&new_val, &cloned_child);
	} ZEND_HASH_FOREACH_END();

	zend_update_property(list_tag_ce, Z_OBJ_P(getThis()), "value", sizeof("value") - 1, &new_val);
	zval_ptr_dtor(&new_val);
}

PHP_METHOD(pocketmine_nbt_tag_ListTag, makeCopy) {
	ZEND_PARSE_PARAMETERS_NONE();
	zend_object* new_obj = Z_OBJ_HT_P(getThis())->clone_obj(NBT_CALL_OBJ(getThis()));
	RETURN_OBJ(new_obj);
}

PHP_METHOD(pocketmine_nbt_tag_ListTag, getIterator) {
	ZEND_PARSE_PARAMETERS_NONE();
	zval rv;
	zval* val = zend_read_property(list_tag_ce, Z_OBJ_P(getThis()), "value", sizeof("value") - 1, 1, &rv);
	object_init_ex(return_value, spl_ce_ArrayIterator);
	call_method_1(return_value, "__construct", NULL, val);
}

PHP_METHOD(pocketmine_nbt_tag_ListTag, equals) {
	zval* that;
	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_OBJECT_OF_CLASS(that, tag_ce)
	ZEND_PARSE_PARAMETERS_END();

	if (!instanceof_function(Z_OBJCE_P(that), list_tag_ce)) {
		RETURN_FALSE;
	}

	zval rv1, rv2;
	zval* val1 = zend_read_property(list_tag_ce, Z_OBJ_P(getThis()), "value", sizeof("value") - 1, 1, &rv1);
	zval* val2 = zend_read_property(list_tag_ce, Z_OBJ_P(that), "value", sizeof("value") - 1, 1, &rv2);

	uint32_t c1 = zend_hash_num_elements(Z_ARRVAL_P(val1));
	uint32_t c2 = zend_hash_num_elements(Z_ARRVAL_P(val2));
	if (c1 != c2) {
		RETURN_FALSE;
	}

	for (uint32_t i = 0; i < c1; ++i) {
		zval* e1 = zend_hash_index_find(Z_ARRVAL_P(val1), i);
		zval* e2 = zend_hash_index_find(Z_ARRVAL_P(val2), i);

		zval eq_zv;
		call_method_1(e1, "equals", &eq_zv, e2);
		zend_bool eq = zend_is_true(&eq_zv);
		zval_ptr_dtor(&eq_zv);
		if (!eq) {
			RETURN_FALSE;
		}
	}

	RETURN_TRUE;
}

PHP_METHOD(pocketmine_nbt_tag_ListTag, read) {
	zval* reader;
	zval* tracker;
	ZEND_PARSE_PARAMETERS_START(2, 2)
		Z_PARAM_OBJECT_OF_CLASS(reader, nbt_stream_reader_ce)
		Z_PARAM_OBJECT_OF_CLASS(tracker, reader_tracker_ce)
	ZEND_PARSE_PARAMETERS_END();

	zval type_zv, size_zv;
	call_method_0(reader, "readByte", &type_zv);
	call_method_0(reader, "readInt", &size_zv);

	zend_long tagType = zval_get_long(&type_zv);
	zend_long size = zval_get_long(&size_zv);
	zval_ptr_dtor(&type_zv);
	zval_ptr_dtor(&size_zv);

	zval list_arr;
	array_init(&list_arr);

	if (size > 0) {
		if (tagType == NBT_TAG_END) {
			zval_ptr_dtor(&list_arr);
			throw_nbt_data_exception("Unexpected non-empty list of TAG_End");
			RETURN_THROWS();
		}

		for (zend_long i = 0; i < size; ++i) {
			zval tag_zv;
			nbt_create_tag_from_type(tagType, reader, tracker, &tag_zv);
			if (EG(exception)) {
				zval_ptr_dtor(&list_arr);
				RETURN_THROWS();
			}
			add_next_index_zval(&list_arr, &tag_zv);
		}
	}

	object_init_ex(return_value, list_tag_ce);
	zend_update_property(list_tag_ce, Z_OBJ_P(return_value), "value", sizeof("value") - 1, &list_arr);
	zend_update_property_long(list_tag_ce, Z_OBJ_P(return_value), "tagType", sizeof("tagType") - 1, tagType);
	zval_ptr_dtor(&list_arr);
}

static const zend_function_entry list_tag_methods[] = {
	PHP_ME(pocketmine_nbt_tag_ListTag, __construct, arginfo_list_construct, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_tag_ListTag, getValue, arginfo_read_array, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_tag_ListTag, getAllValues, arginfo_read_array, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_tag_ListTag, cast, arginfo_list_cast, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_tag_ListTag, count, arginfo_tag_get_type, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_tag_ListTag, getCount, arginfo_tag_get_type, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_tag_ListTag, push, arginfo_list_push, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_tag_ListTag, pop, arginfo_list_pop, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_tag_ListTag, unshift, arginfo_list_push, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_tag_ListTag, shift, arginfo_list_pop, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_tag_ListTag, insert, arginfo_list_insert, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_tag_ListTag, remove, arginfo_list_remove, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_tag_ListTag, get, arginfo_list_get, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_tag_ListTag, first, arginfo_list_pop, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_tag_ListTag, last, arginfo_list_pop, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_tag_ListTag, set, arginfo_list_insert, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_tag_ListTag, isset, arginfo_list_isset, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_tag_ListTag, empty, arginfo_list_empty, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_tag_ListTag, getType, arginfo_tag_get_type, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_tag_ListTag, getTypeName, arginfo_tag_get_type_name, ZEND_ACC_PROTECTED)
	PHP_ME(pocketmine_nbt_tag_ListTag, getTagType, arginfo_tag_get_type, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_tag_ListTag, setTagType, arginfo_list_set_tag_type, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_tag_ListTag, read, arginfo_list_read, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	PHP_ME(pocketmine_nbt_tag_ListTag, write, arginfo_tag_write, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_tag_ListTag, stringifyValue, arginfo_stringify_value, ZEND_ACC_PROTECTED)
	PHP_ME(pocketmine_nbt_tag_ListTag, __clone, arginfo_tag_clone, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_tag_ListTag, makeCopy, arginfo_tag_make_copy, ZEND_ACC_PROTECTED)
	PHP_ME(pocketmine_nbt_tag_ListTag, getIterator, arginfo_tag_get_iterator, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_tag_ListTag, equals, arginfo_tag_equals, ZEND_ACC_PUBLIC)
	// Trait methods
	PHP_ME(pocketmine_nbt_tag_NoDynamicFieldsTrait, __get, arginfo_dynamic_field_get, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_tag_NoDynamicFieldsTrait, __set, arginfo_dynamic_field_set, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_tag_NoDynamicFieldsTrait, __isset, arginfo_dynamic_field_get, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_tag_NoDynamicFieldsTrait, __unset, arginfo_dynamic_field_get, ZEND_ACC_PUBLIC)
	PHP_FE_END
};

// -------------------------------------------------------------
// CompoundTag
// -------------------------------------------------------------
ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_compound_create, 0, 0, pocketmine\\nbt\\tag\\CompoundTag, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_compound_get_tag, 0, 1, pocketmine\\nbt\\tag\\Tag, 1)
	ZEND_ARG_TYPE_INFO(0, name, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_compound_get_list_tag, 0, 1, pocketmine\\nbt\\tag\\ListTag, 1)
	ZEND_ARG_TYPE_INFO(0, name, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, tagClass, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_compound_get_compound_tag, 0, 1, pocketmine\\nbt\\tag\\CompoundTag, 1)
	ZEND_ARG_TYPE_INFO(0, name, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_compound_set_tag, 0, 2, pocketmine\\nbt\\tag\\CompoundTag, 0)
	ZEND_ARG_TYPE_INFO(0, name, IS_STRING, 0)
	ZEND_ARG_OBJ_INFO(0, tag, pocketmine\\nbt\\tag\\Tag, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_compound_remove_tag, 0, 0, IS_VOID, 0)
	ZEND_ARG_VARIADIC_TYPE_INFO(0, names, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_compound_get_byte, 0, 1, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, name, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, default, IS_LONG, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_compound_get_float, 0, 1, IS_DOUBLE, 0)
	ZEND_ARG_TYPE_INFO(0, name, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, default, IS_DOUBLE, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_compound_get_string, 0, 1, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, name, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, default, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_compound_get_array, 0, 1, IS_ARRAY, 0)
	ZEND_ARG_TYPE_INFO(0, name, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, default, IS_ARRAY, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_compound_set_byte, 0, 2, pocketmine\\nbt\\tag\\CompoundTag, 0)
	ZEND_ARG_TYPE_INFO(0, name, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, value, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_compound_set_float, 0, 2, pocketmine\\nbt\\tag\\CompoundTag, 0)
	ZEND_ARG_TYPE_INFO(0, name, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, value, IS_DOUBLE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_compound_set_string, 0, 2, pocketmine\\nbt\\tag\\CompoundTag, 0)
	ZEND_ARG_TYPE_INFO(0, name, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, value, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_compound_set_array, 0, 2, pocketmine\\nbt\\tag\\CompoundTag, 0)
	ZEND_ARG_TYPE_INFO(0, name, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, value, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_compound_merge, 0, 1, pocketmine\\nbt\\tag\\CompoundTag, 0)
	ZEND_ARG_OBJ_INFO(0, other, pocketmine\\nbt\\tag\\CompoundTag, 0)
ZEND_END_ARG_INFO()

PHP_METHOD(pocketmine_nbt_tag_CompoundTag, __construct) {
	ZEND_PARSE_PARAMETERS_START(0, 0)
	ZEND_PARSE_PARAMETERS_END();
	check_arg_count("pocketmine\\nbt\\tag\\CompoundTag::__construct", EX(func)->common.num_args, 0);

	zval empty_arr;
	array_init(&empty_arr);
	zend_update_property(compound_tag_ce, Z_OBJ_P(getThis()), "value", sizeof("value") - 1, &empty_arr);
	zval_ptr_dtor(&empty_arr);
}

PHP_METHOD(pocketmine_nbt_tag_CompoundTag, create) {
	ZEND_PARSE_PARAMETERS_NONE();
	object_init_ex(return_value, compound_tag_ce);
	call_method_0(return_value, "__construct", NULL);
}

PHP_METHOD(pocketmine_nbt_tag_CompoundTag, count) {
	ZEND_PARSE_PARAMETERS_NONE();
	zval rv;
	zval* val = zend_read_property(compound_tag_ce, Z_OBJ_P(getThis()), "value", sizeof("value") - 1, 1, &rv);
	RETURN_LONG(zend_hash_num_elements(Z_ARRVAL_P(val)));
}

PHP_METHOD(pocketmine_nbt_tag_CompoundTag, getCount) {
	ZEND_PARSE_PARAMETERS_NONE();
	zval rv;
	zval* val = zend_read_property(compound_tag_ce, Z_OBJ_P(getThis()), "value", sizeof("value") - 1, 1, &rv);
	RETURN_LONG(zend_hash_num_elements(Z_ARRVAL_P(val)));
}

PHP_METHOD(pocketmine_nbt_tag_CompoundTag, getValue) {
	ZEND_PARSE_PARAMETERS_NONE();
	zval rv;
	zval* val = zend_read_property(compound_tag_ce, Z_OBJ_P(getThis()), "value", sizeof("value") - 1, 1, &rv);
	RETURN_ZVAL(val, 1, 0);
}

PHP_METHOD(pocketmine_nbt_tag_CompoundTag, getTag) {
	zend_string* name;
	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_STR(name)
	ZEND_PARSE_PARAMETERS_END();

	zval rv;
	zval* val = zend_read_property(compound_tag_ce, Z_OBJ_P(getThis()), "value", sizeof("value") - 1, 1, &rv);
	zval* tag = zend_hash_find(Z_ARRVAL_P(val), name);
	if (tag) {
		RETURN_ZVAL(tag, 1, 0);
	}
	RETURN_NULL();
}

PHP_METHOD(pocketmine_nbt_tag_CompoundTag, getListTag) {
	zend_string* name;
	zend_string* tagClass = NULL;
	ZEND_PARSE_PARAMETERS_START(1, 2)
		Z_PARAM_STR(name)
		Z_PARAM_OPTIONAL
		Z_PARAM_STR(tagClass)
	ZEND_PARSE_PARAMETERS_END();

	zval tag_zv, arg;
	ZVAL_STR(&arg, name);
	call_method_1(getThis(), "getTag", &tag_zv, &arg);

	if (Z_TYPE(tag_zv) != IS_NULL) {
		if (!instanceof_function(Z_OBJCE(tag_zv), list_tag_ce)) {
			const char* expected = ZSTR_VAL(list_tag_ce->name);
			const char* actual = ZSTR_VAL(Z_OBJCE(tag_zv)->name);
			zval_ptr_dtor(&tag_zv);
			throw_unexpected_tag_type_exception("Expected a tag of type %s, got %s", expected, actual);
			RETURN_THROWS();
		}

		if (tagClass) {
			zval cast_zv, cast_arg;
			ZVAL_STR(&cast_arg, tagClass);
			call_method_1(&tag_zv, "cast", &cast_zv, &cast_arg);
			zval_ptr_dtor(&tag_zv);

			if (Z_TYPE(cast_zv) == IS_NULL) {
				throw_unexpected_tag_type_exception("Unable to cast list to ListTag<%s>", ZSTR_VAL(tagClass));
				RETURN_THROWS();
			}
			RETURN_ZVAL(&cast_zv, 0, 0);
		} else {
			RETURN_ZVAL(&tag_zv, 0, 0);
		}
	}
	RETURN_NULL();
}

PHP_METHOD(pocketmine_nbt_tag_CompoundTag, getCompoundTag) {
	zend_string* name;
	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_STR(name)
	ZEND_PARSE_PARAMETERS_END();

	zval arg, tag_zv;
	ZVAL_STR(&arg, name);
	call_method_1(getThis(), "getTag", &tag_zv, &arg);

	if (Z_TYPE(tag_zv) != IS_NULL) {
		if (!instanceof_function(Z_OBJCE(tag_zv), compound_tag_ce)) {
			const char* expected = ZSTR_VAL(compound_tag_ce->name);
			const char* actual = ZSTR_VAL(Z_OBJCE(tag_zv)->name);
			zval_ptr_dtor(&tag_zv);
			throw_unexpected_tag_type_exception("Expected a tag of type %s, got %s", expected, actual);
			RETURN_THROWS();
		}
		RETURN_ZVAL(&tag_zv, 0, 0);
	}
	RETURN_NULL();
}

PHP_METHOD(pocketmine_nbt_tag_CompoundTag, setTag) {
	zend_string* name;
	zval* tag;
	ZEND_PARSE_PARAMETERS_START(2, 2)
		Z_PARAM_STR(name)
		Z_PARAM_OBJECT_OF_CLASS(tag, tag_ce)
	ZEND_PARSE_PARAMETERS_END();

	if (ZSTR_LEN(name) > 32767) {
		zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0,
			"Tag name must be at most %d bytes, but got %d bytes", 32767, (int)ZSTR_LEN(name));
		RETURN_THROWS();
	}

	zval rv;
	zval* val = zend_read_property(compound_tag_ce, Z_OBJ_P(getThis()), "value", sizeof("value") - 1, 1, &rv);
	Z_TRY_ADDREF_P(tag);
	zend_hash_update(Z_ARRVAL_P(val), name, tag);

	RETURN_ZVAL(getThis(), 1, 0);
}

PHP_METHOD(pocketmine_nbt_tag_CompoundTag, removeTag) {
	zval* names;
	int num_names;
	ZEND_PARSE_PARAMETERS_START(0, -1)
		Z_PARAM_VARIADIC('*', names, num_names)
	ZEND_PARSE_PARAMETERS_END();

	zval rv;
	zval* val = zend_read_property(compound_tag_ce, Z_OBJ_P(getThis()), "value", sizeof("value") - 1, 1, &rv);

	for (int i = 0; i < num_names; ++i) {
		if (Z_TYPE(names[i]) == IS_STRING) {
			zend_hash_del(Z_ARRVAL_P(val), Z_STR(names[i]));
		}
	}
}

// Helper for typed getters
static void compound_get_tag_value(INTERNAL_FUNCTION_PARAMETERS, zend_class_entry* expected_ce) {
	zend_string* name;
	zval* default_val = NULL;
	ZEND_PARSE_PARAMETERS_START(1, 2)
		Z_PARAM_STR(name)
		Z_PARAM_OPTIONAL
		Z_PARAM_ZVAL(default_val)
	ZEND_PARSE_PARAMETERS_END();

	zval rv;
	zval* val = zend_read_property(compound_tag_ce, Z_OBJ_P(getThis()), "value", sizeof("value") - 1, 1, &rv);
	zval* tag = zend_hash_find(Z_ARRVAL_P(val), name);

	if (tag) {
		if (instanceof_function(Z_OBJCE_P(tag), expected_ce)) {
			call_method_0(tag, "getValue", return_value);
			return;
		}
		throw_unexpected_tag_type_exception("Expected a tag of type %s, got %s",
			ZSTR_VAL(expected_ce->name), ZSTR_VAL(Z_OBJCE_P(tag)->name));
		RETURN_THROWS();
	}

	if (!default_val || Z_TYPE_P(default_val) == IS_NULL) {
		throw_no_such_tag_exception("Tag \"%s\" does not exist", ZSTR_VAL(name));
		RETURN_THROWS();
	}

	RETURN_ZVAL(default_val, 1, 0);
}

PHP_METHOD(pocketmine_nbt_tag_CompoundTag, getByte) { compound_get_tag_value(INTERNAL_FUNCTION_PARAM_PASSTHRU, byte_tag_ce); }
PHP_METHOD(pocketmine_nbt_tag_CompoundTag, getShort) { compound_get_tag_value(INTERNAL_FUNCTION_PARAM_PASSTHRU, short_tag_ce); }
PHP_METHOD(pocketmine_nbt_tag_CompoundTag, getInt) { compound_get_tag_value(INTERNAL_FUNCTION_PARAM_PASSTHRU, int_tag_ce); }
PHP_METHOD(pocketmine_nbt_tag_CompoundTag, getLong) { compound_get_tag_value(INTERNAL_FUNCTION_PARAM_PASSTHRU, long_tag_ce); }
PHP_METHOD(pocketmine_nbt_tag_CompoundTag, getFloat) { compound_get_tag_value(INTERNAL_FUNCTION_PARAM_PASSTHRU, float_tag_ce); }
PHP_METHOD(pocketmine_nbt_tag_CompoundTag, getDouble) { compound_get_tag_value(INTERNAL_FUNCTION_PARAM_PASSTHRU, double_tag_ce); }
PHP_METHOD(pocketmine_nbt_tag_CompoundTag, getByteArray) { compound_get_tag_value(INTERNAL_FUNCTION_PARAM_PASSTHRU, byte_array_tag_ce); }
PHP_METHOD(pocketmine_nbt_tag_CompoundTag, getString) { compound_get_tag_value(INTERNAL_FUNCTION_PARAM_PASSTHRU, string_tag_ce); }
PHP_METHOD(pocketmine_nbt_tag_CompoundTag, getIntArray) { compound_get_tag_value(INTERNAL_FUNCTION_PARAM_PASSTHRU, int_array_tag_ce); }

// Helper for typed setters
static void compound_set_tag_helper(INTERNAL_FUNCTION_PARAMETERS, zend_class_entry* tag_class_ce) {
	zend_string* name;
	zval* value;
	ZEND_PARSE_PARAMETERS_START(2, 2)
		Z_PARAM_STR(name)
		Z_PARAM_ZVAL(value)
	ZEND_PARSE_PARAMETERS_END();

	zval new_tag;
	object_init_ex(&new_tag, tag_class_ce);
	call_method_1(&new_tag, "__construct", NULL, value);
	if (EG(exception)) {
		zval_ptr_dtor(&new_tag);
		RETURN_THROWS();
	}

	zval name_arg;
	ZVAL_STR(&name_arg, name);
	call_method_2(getThis(), "setTag", return_value, &name_arg, &new_tag);
	zval_ptr_dtor(&new_tag);
}

PHP_METHOD(pocketmine_nbt_tag_CompoundTag, setByte) { compound_set_tag_helper(INTERNAL_FUNCTION_PARAM_PASSTHRU, byte_tag_ce); }
PHP_METHOD(pocketmine_nbt_tag_CompoundTag, setShort) { compound_set_tag_helper(INTERNAL_FUNCTION_PARAM_PASSTHRU, short_tag_ce); }
PHP_METHOD(pocketmine_nbt_tag_CompoundTag, setInt) { compound_set_tag_helper(INTERNAL_FUNCTION_PARAM_PASSTHRU, int_tag_ce); }
PHP_METHOD(pocketmine_nbt_tag_CompoundTag, setLong) { compound_set_tag_helper(INTERNAL_FUNCTION_PARAM_PASSTHRU, long_tag_ce); }
PHP_METHOD(pocketmine_nbt_tag_CompoundTag, setFloat) { compound_set_tag_helper(INTERNAL_FUNCTION_PARAM_PASSTHRU, float_tag_ce); }
PHP_METHOD(pocketmine_nbt_tag_CompoundTag, setDouble) { compound_set_tag_helper(INTERNAL_FUNCTION_PARAM_PASSTHRU, double_tag_ce); }
PHP_METHOD(pocketmine_nbt_tag_CompoundTag, setByteArray) { compound_set_tag_helper(INTERNAL_FUNCTION_PARAM_PASSTHRU, byte_array_tag_ce); }
PHP_METHOD(pocketmine_nbt_tag_CompoundTag, setString) { compound_set_tag_helper(INTERNAL_FUNCTION_PARAM_PASSTHRU, string_tag_ce); }
PHP_METHOD(pocketmine_nbt_tag_CompoundTag, setIntArray) { compound_set_tag_helper(INTERNAL_FUNCTION_PARAM_PASSTHRU, int_array_tag_ce); }

PHP_METHOD(pocketmine_nbt_tag_CompoundTag, getType) {
	ZEND_PARSE_PARAMETERS_NONE();
	RETURN_LONG(NBT_TAG_COMPOUND);
}

PHP_METHOD(pocketmine_nbt_tag_CompoundTag, getTypeName) {
	ZEND_PARSE_PARAMETERS_NONE();
	RETURN_STRING("Compound");
}

PHP_METHOD(pocketmine_nbt_tag_CompoundTag, write) {
	zval* writer;
	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_OBJECT_OF_CLASS(writer, nbt_stream_writer_ce)
	ZEND_PARSE_PARAMETERS_END();

	zval rv;
	zval* val = zend_read_property(compound_tag_ce, Z_OBJ_P(getThis()), "value", sizeof("value") - 1, 1, &rv);

	zend_string* key;
	zend_ulong num_key;
	zval* entry;

	ZEND_HASH_FOREACH_KEY_VAL(Z_ARRVAL_P(val), num_key, key, entry) {
		char numeric_key[32];
		const char* str_key;
		size_t str_key_len;
		if (key) {
			str_key = ZSTR_VAL(key);
			str_key_len = ZSTR_LEN(key);
		} else {
			str_key_len = snprintf(numeric_key, sizeof(numeric_key), "%llu", (unsigned long long)num_key);
			str_key = numeric_key;
		}

		zval type_zv;
		call_method_0(entry, "getType", &type_zv);
		zval dummy;
		call_method_1(writer, "writeByte", &dummy, &type_zv);
		zval_ptr_dtor(&dummy);
		zval_ptr_dtor(&type_zv);

		zval name_zv;
		ZVAL_STRINGL(&name_zv, str_key, str_key_len);
		call_method_1(writer, "writeString", &dummy, &name_zv);
		zval_ptr_dtor(&dummy);
		zval_ptr_dtor(&name_zv);

		call_method_1(entry, "write", &dummy, writer);
		zval_ptr_dtor(&dummy);
	} ZEND_HASH_FOREACH_END();

	zval end_zv;
	ZVAL_LONG(&end_zv, NBT_TAG_END);
	zval dummy;
	call_method_1(writer, "writeByte", &dummy, &end_zv);
	zval_ptr_dtor(&dummy);
}

PHP_METHOD(pocketmine_nbt_tag_CompoundTag, stringifyValue) {
	zend_long indentation;
	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_LONG(indentation)
	ZEND_PARSE_PARAMETERS_END();

	zval rv;
	zval* val = zend_read_property(compound_tag_ce, Z_OBJ_P(getThis()), "value", sizeof("value") - 1, 1, &rv);

	smart_str output = {0};
	smart_str_appends(&output, "{\n");

	zend_string* key;
	zend_ulong num_key;
	zval* entry;

	ZEND_HASH_FOREACH_KEY_VAL(Z_ARRVAL_P(val), num_key, key, entry) {
		char numeric_key[32];
		const char* str_key;
		size_t str_key_len;
		if (key) {
			str_key = ZSTR_VAL(key);
			str_key_len = ZSTR_LEN(key);
		} else {
			str_key_len = snprintf(numeric_key, sizeof(numeric_key), "%llu", (unsigned long long)num_key);
			str_key = numeric_key;
		}

		zval str_zv;
		zval arg;
		ZVAL_LONG(&arg, indentation + 1);
		call_method_1(entry, "toString", &str_zv, &arg);

		for (zend_long i = 0; i < (indentation + 1) * 2; ++i) smart_str_appendc(&output, ' ');
		smart_str_appendc(&output, '"');
		smart_str_appendl(&output, str_key, str_key_len);
		smart_str_appends(&output, "\" => ");
		smart_str_appendl(&output, Z_STRVAL(str_zv), Z_STRLEN(str_zv));
		smart_str_appendc(&output, '\n');
		zval_ptr_dtor(&str_zv);
	} ZEND_HASH_FOREACH_END();

	for (zend_long i = 0; i < indentation * 2; ++i) smart_str_appendc(&output, ' ');
	smart_str_appendc(&output, '}');
	smart_str_0(&output);
	RETURN_STR(output.s);
}

PHP_METHOD(pocketmine_nbt_tag_CompoundTag, __clone) {
	zval rv;
	zval* val = zend_read_property(compound_tag_ce, Z_OBJ_P(getThis()), "value", sizeof("value") - 1, 1, &rv);
	uint32_t count = zend_hash_num_elements(Z_ARRVAL_P(val));

	zval new_val;
	array_init_size(&new_val, count);

	zend_string* key;
	zend_ulong num_key;
	zval* entry;

	ZEND_HASH_FOREACH_KEY_VAL(Z_ARRVAL_P(val), num_key, key, entry) {
		zval cloned_child;
		call_method_0(entry, "safeClone", &cloned_child);
		if (EG(exception)) {
			zval_ptr_dtor(&new_val);
			return;
		}
		if (key) {
			zend_hash_update(Z_ARRVAL(new_val), key, &cloned_child);
		} else {
			zend_string* s = zend_long_to_str(num_key);
			zend_hash_update(Z_ARRVAL(new_val), s, &cloned_child);
			zend_string_release(s);
		}
	} ZEND_HASH_FOREACH_END();

	zend_update_property(compound_tag_ce, Z_OBJ_P(getThis()), "value", sizeof("value") - 1, &new_val);
	zval_ptr_dtor(&new_val);
}

PHP_METHOD(pocketmine_nbt_tag_CompoundTag, makeCopy) {
	ZEND_PARSE_PARAMETERS_NONE();
	zend_object* new_obj = Z_OBJ_HT_P(getThis())->clone_obj(NBT_CALL_OBJ(getThis()));
	RETURN_OBJ(new_obj);
}

PHP_METHOD(pocketmine_nbt_tag_CompoundTag, getIterator) {
	ZEND_PARSE_PARAMETERS_NONE();
	zval rv;
	zval* val = zend_read_property(compound_tag_ce, Z_OBJ_P(getThis()), "value", sizeof("value") - 1, 1, &rv);
	object_init_ex(return_value, spl_ce_ArrayIterator);
	call_method_1(return_value, "__construct", NULL, val);
}

PHP_METHOD(pocketmine_nbt_tag_CompoundTag, equals) {
	zval* that;
	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_OBJECT_OF_CLASS(that, tag_ce)
	ZEND_PARSE_PARAMETERS_END();

	if (!instanceof_function(Z_OBJCE_P(that), compound_tag_ce)) {
		RETURN_FALSE;
	}

	zval rv1, rv2;
	zval* val1 = zend_read_property(compound_tag_ce, Z_OBJ_P(getThis()), "value", sizeof("value") - 1, 1, &rv1);
	zval* val2 = zend_read_property(compound_tag_ce, Z_OBJ_P(that), "value", sizeof("value") - 1, 1, &rv2);

	if (zend_hash_num_elements(Z_ARRVAL_P(val1)) != zend_hash_num_elements(Z_ARRVAL_P(val2))) {
		RETURN_FALSE;
	}

	zend_string* key;
	zend_ulong num_key;
	zval* entry;

	ZEND_HASH_FOREACH_KEY_VAL(Z_ARRVAL_P(val1), num_key, key, entry) {
		zval* other_entry;
		if (key) {
			other_entry = zend_hash_find(Z_ARRVAL_P(val2), key);
		} else {
			other_entry = zend_hash_index_find(Z_ARRVAL_P(val2), num_key);
		}

		if (!other_entry) {
			RETURN_FALSE;
		}

		zval eq_zv;
		call_method_1(entry, "equals", &eq_zv, other_entry);
		zend_bool eq = zend_is_true(&eq_zv);
		zval_ptr_dtor(&eq_zv);
		if (!eq) {
			RETURN_FALSE;
		}
	} ZEND_HASH_FOREACH_END();

	RETURN_TRUE;
}

PHP_METHOD(pocketmine_nbt_tag_CompoundTag, merge) {
	zval* other;
	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_OBJECT_OF_CLASS(other, compound_tag_ce)
	ZEND_PARSE_PARAMETERS_END();

	// clone getThis()
	call_method_0(getThis(), "makeCopy", return_value);
	if (EG(exception)) {
		return;
	}

	zval rv;
	zval* other_val = zend_read_property(compound_tag_ce, Z_OBJ_P(other), "value", sizeof("value") - 1, 1, &rv);

	zend_string* key;
	zend_ulong num_key;
	zval* entry;

	ZEND_HASH_FOREACH_KEY_VAL(Z_ARRVAL_P(other_val), num_key, key, entry) {
		zval cloned;
		call_method_0(entry, "safeClone", &cloned);
		if (EG(exception)) {
			return;
		}

		zval name_arg;
		if (key) {
			ZVAL_STR(&name_arg, key);
		} else {
			zend_string* s = zend_long_to_str(num_key);
			ZVAL_STR(&name_arg, s);
		}
		call_method_2(return_value, "setTag", NULL, &name_arg, &cloned);
		zval_ptr_dtor(&cloned);
		if (!key) {
			zend_string_release(Z_STR(name_arg));
		}
	} ZEND_HASH_FOREACH_END();
}

PHP_METHOD(pocketmine_nbt_tag_CompoundTag, read) {
	zval* reader;
	zval* tracker;
	ZEND_PARSE_PARAMETERS_START(2, 2)
		Z_PARAM_OBJECT_OF_CLASS(reader, nbt_stream_reader_ce)
		Z_PARAM_OBJECT_OF_CLASS(tracker, reader_tracker_ce)
	ZEND_PARSE_PARAMETERS_END();

	object_init_ex(return_value, compound_tag_ce);
	call_method_0(return_value, "__construct", NULL);

	/* Read the primitive values directly from the stream. */
	while (1) {
		zval type_zv;
		call_method_0(reader, "readByte", &type_zv);
		zend_long type = zval_get_long(&type_zv);
		zval_ptr_dtor(&type_zv);

		if (type == NBT_TAG_END) {
			break;
		}

		zval name_zv;
		call_method_0(reader, "readString", &name_zv);

		zval tag_zv;
		nbt_create_tag_from_type(type, reader, tracker, &tag_zv);
		if (EG(exception)) {
			zval_ptr_dtor(&name_zv);
			RETURN_THROWS();
		}

		zval existing;
		call_method_1(return_value, "getTag", &existing, &name_zv);
		if (Z_TYPE(existing) == IS_NULL) {
			call_method_2(return_value, "setTag", NULL, &name_zv, &tag_zv);
		}
		zval_ptr_dtor(&existing);
		zval_ptr_dtor(&tag_zv);
		zval_ptr_dtor(&name_zv);
	}
}

static const zend_function_entry compound_tag_methods[] = {
	PHP_ME(pocketmine_nbt_tag_CompoundTag, __construct, arginfo_compound_construct, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_tag_CompoundTag, create, arginfo_compound_create, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	PHP_ME(pocketmine_nbt_tag_CompoundTag, count, arginfo_tag_get_type, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_tag_CompoundTag, getCount, arginfo_tag_get_type, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_tag_CompoundTag, getValue, arginfo_read_array, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_tag_CompoundTag, getTag, arginfo_compound_get_tag, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_tag_CompoundTag, getListTag, arginfo_compound_get_list_tag, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_tag_CompoundTag, getCompoundTag, arginfo_compound_get_compound_tag, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_tag_CompoundTag, setTag, arginfo_compound_set_tag, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_tag_CompoundTag, removeTag, arginfo_compound_remove_tag, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_tag_CompoundTag, getByte, arginfo_compound_get_byte, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_tag_CompoundTag, getShort, arginfo_compound_get_byte, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_tag_CompoundTag, getInt, arginfo_compound_get_byte, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_tag_CompoundTag, getLong, arginfo_compound_get_byte, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_tag_CompoundTag, getFloat, arginfo_compound_get_float, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_tag_CompoundTag, getDouble, arginfo_compound_get_float, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_tag_CompoundTag, getByteArray, arginfo_compound_get_string, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_tag_CompoundTag, getString, arginfo_compound_get_string, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_tag_CompoundTag, getIntArray, arginfo_compound_get_array, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_tag_CompoundTag, setByte, arginfo_compound_set_byte, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_tag_CompoundTag, setShort, arginfo_compound_set_byte, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_tag_CompoundTag, setInt, arginfo_compound_set_byte, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_tag_CompoundTag, setLong, arginfo_compound_set_byte, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_tag_CompoundTag, setFloat, arginfo_compound_set_float, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_tag_CompoundTag, setDouble, arginfo_compound_set_float, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_tag_CompoundTag, setByteArray, arginfo_compound_set_string, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_tag_CompoundTag, setString, arginfo_compound_set_string, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_tag_CompoundTag, setIntArray, arginfo_compound_set_array, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_tag_CompoundTag, getType, arginfo_tag_get_type, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_tag_CompoundTag, getTypeName, arginfo_tag_get_type_name, ZEND_ACC_PROTECTED)
	PHP_ME(pocketmine_nbt_tag_CompoundTag, read, arginfo_list_read, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	PHP_ME(pocketmine_nbt_tag_CompoundTag, write, arginfo_tag_write, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_tag_CompoundTag, stringifyValue, arginfo_stringify_value, ZEND_ACC_PROTECTED)
	PHP_ME(pocketmine_nbt_tag_CompoundTag, __clone, arginfo_tag_clone, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_tag_CompoundTag, makeCopy, arginfo_tag_make_copy, ZEND_ACC_PROTECTED)
	PHP_ME(pocketmine_nbt_tag_CompoundTag, getIterator, arginfo_tag_get_iterator, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_tag_CompoundTag, equals, arginfo_tag_equals, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_tag_CompoundTag, merge, arginfo_compound_merge, ZEND_ACC_PUBLIC)
	// Trait methods
	PHP_ME(pocketmine_nbt_tag_NoDynamicFieldsTrait, __get, arginfo_dynamic_field_get, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_tag_NoDynamicFieldsTrait, __set, arginfo_dynamic_field_set, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_tag_NoDynamicFieldsTrait, __isset, arginfo_dynamic_field_get, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_tag_NoDynamicFieldsTrait, __unset, arginfo_dynamic_field_get, ZEND_ACC_PUBLIC)
	PHP_FE_END
};

// -------------------------------------------------------------
// TreeRoot
// -------------------------------------------------------------
ZEND_BEGIN_ARG_INFO_EX(arginfo_tree_root_construct, 0, 0, 1)
	ZEND_ARG_OBJ_INFO(0, root, pocketmine\\nbt\\tag\\Tag, 0)
	ZEND_ARG_TYPE_INFO(0, name, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_tree_root_must_get_compound, 0, 0, pocketmine\\nbt\\tag\\CompoundTag, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_tree_root_equals, 0, 1, _IS_BOOL, 0)
	ZEND_ARG_OBJ_INFO(0, that, pocketmine\\nbt\\TreeRoot, 0)
ZEND_END_ARG_INFO()

PHP_METHOD(pocketmine_nbt_TreeRoot, __construct) {
	zval* root;
	zend_string* name = NULL;
	ZEND_PARSE_PARAMETERS_START(1, 2)
		Z_PARAM_OBJECT_OF_CLASS(root, tag_ce)
		Z_PARAM_OPTIONAL
		Z_PARAM_STR(name)
	ZEND_PARSE_PARAMETERS_END();

	if (name && ZSTR_LEN(name) > 32767) {
		zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0,
			"Tag name must be at most %d bytes, but got %d bytes", 32767, (int)ZSTR_LEN(name));
		RETURN_THROWS();
	}

	zend_update_property(tree_root_ce, Z_OBJ_P(getThis()), "root", sizeof("root") - 1, root);
	if (name) {
		zend_update_property_str(tree_root_ce, Z_OBJ_P(getThis()), "name", sizeof("name") - 1, name);
	} else {
		zend_update_property_string(tree_root_ce, Z_OBJ_P(getThis()), "name", sizeof("name") - 1, "");
	}
}

PHP_METHOD(pocketmine_nbt_TreeRoot, getTag) {
	ZEND_PARSE_PARAMETERS_NONE();
	zval rv;
	zval* root = zend_read_property(tree_root_ce, Z_OBJ_P(getThis()), "root", sizeof("root") - 1, 1, &rv);
	RETURN_ZVAL(root, 1, 0);
}

PHP_METHOD(pocketmine_nbt_TreeRoot, mustGetCompoundTag) {
	ZEND_PARSE_PARAMETERS_NONE();
	zval rv;
	zval* root = zend_read_property(tree_root_ce, Z_OBJ_P(getThis()), "root", sizeof("root") - 1, 1, &rv);
	if (instanceof_function(Z_OBJCE_P(root), compound_tag_ce)) {
		RETURN_ZVAL(root, 1, 0);
	}
	throw_unexpected_tag_type_exception("Root is not a TAG_Compound");
	RETURN_THROWS();
}

PHP_METHOD(pocketmine_nbt_TreeRoot, getName) {
	ZEND_PARSE_PARAMETERS_NONE();
	zval rv;
	zval* name = zend_read_property(tree_root_ce, Z_OBJ_P(getThis()), "name", sizeof("name") - 1, 1, &rv);
	RETURN_ZVAL(name, 1, 0);
}

PHP_METHOD(pocketmine_nbt_TreeRoot, equals) {
	zval* that;
	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_OBJECT_OF_CLASS(that, tree_root_ce)
	ZEND_PARSE_PARAMETERS_END();

	zval rv1, rv2;
	zval* name1 = zend_read_property(tree_root_ce, Z_OBJ_P(getThis()), "name", sizeof("name") - 1, 1, &rv1);
	zval* name2 = zend_read_property(tree_root_ce, Z_OBJ_P(that), "name", sizeof("name") - 1, 1, &rv2);

	if (!zend_string_equals(Z_STR_P(name1), Z_STR_P(name2))) {
		RETURN_FALSE;
	}

	zval* root1 = zend_read_property(tree_root_ce, Z_OBJ_P(getThis()), "root", sizeof("root") - 1, 1, &rv1);
	zval* root2 = zend_read_property(tree_root_ce, Z_OBJ_P(that), "root", sizeof("root") - 1, 1, &rv2);

	zval eq_zv;
	call_method_1(root1, "equals", &eq_zv, root2);
	zend_bool eq = zend_is_true(&eq_zv);
	zval_ptr_dtor(&eq_zv);

	RETURN_BOOL(eq);
}

PHP_METHOD(pocketmine_nbt_TreeRoot, __toString) {
	ZEND_PARSE_PARAMETERS_NONE();
	zval rv1, rv2;
	zval* root = zend_read_property(tree_root_ce, Z_OBJ_P(getThis()), "root", sizeof("root") - 1, 1, &rv1);
	zval* name = zend_read_property(tree_root_ce, Z_OBJ_P(getThis()), "name", sizeof("name") - 1, 1, &rv2);

	zval tag_str;
	zval arg;
	ZVAL_LONG(&arg, 1);
	call_method_1(root, "toString", &tag_str, &arg);

	smart_str res = {0};
	smart_str_appends(&res, "ROOT {\n  ");
	if (Z_STRLEN_P(name) > 0) {
		smart_str_appendc(&res, '"');
		smart_str_appendl(&res, Z_STRVAL_P(name), Z_STRLEN_P(name));
		smart_str_appends(&res, "\" => ");
	}
	smart_str_appendl(&res, Z_STRVAL(tag_str), Z_STRLEN(tag_str));
	smart_str_appends(&res, "\n}");
	zval_ptr_dtor(&tag_str);

	smart_str_0(&res);
	RETURN_STR(res.s);
}

static const zend_function_entry tree_root_methods[] = {
	PHP_ME(pocketmine_nbt_TreeRoot, __construct, arginfo_tree_root_construct, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_TreeRoot, getTag, arginfo_tag_make_copy, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_TreeRoot, mustGetCompoundTag, arginfo_tree_root_must_get_compound, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_TreeRoot, getName, arginfo_tag_get_type_name, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_TreeRoot, equals, arginfo_tree_root_equals, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_TreeRoot, __toString, arginfo_tag_magic_to_string, ZEND_ACC_PUBLIC)
	PHP_FE_END
};

// -------------------------------------------------------------
// ReaderTracker
// -------------------------------------------------------------
ZEND_BEGIN_ARG_INFO_EX(arginfo_tracker_construct, 0, 0, 1)
	ZEND_ARG_TYPE_INFO(0, maxDepth, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_tracker_protect_depth, 0, 1, IS_VOID, 0)
	ZEND_ARG_OBJ_INFO(0, execute, Closure, 0)
ZEND_END_ARG_INFO()

PHP_METHOD(pocketmine_nbt_ReaderTracker, __construct) {
	zend_long maxDepth;
	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_LONG(maxDepth)
	ZEND_PARSE_PARAMETERS_END();
	zend_update_property_long(reader_tracker_ce, Z_OBJ_P(getThis()), "maxDepth", sizeof("maxDepth") - 1, maxDepth);
	zend_update_property_long(reader_tracker_ce, Z_OBJ_P(getThis()), "currentDepth", sizeof("currentDepth") - 1, 0);
}

PHP_METHOD(pocketmine_nbt_ReaderTracker, protectDepth) {
	zval* execute;
	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_OBJECT_OF_CLASS(execute, zend_ce_closure)
	ZEND_PARSE_PARAMETERS_END();

	zval rv1, rv2;
	zval* maxDepth_zv = zend_read_property(reader_tracker_ce, Z_OBJ_P(getThis()), "maxDepth", sizeof("maxDepth") - 1, 1, &rv1);
	zval* curDepth_zv = zend_read_property(reader_tracker_ce, Z_OBJ_P(getThis()), "currentDepth", sizeof("currentDepth") - 1, 1, &rv2);

	zend_long maxDepth = zval_get_long(maxDepth_zv);
	zend_long curDepth = zval_get_long(curDepth_zv) + 1;
	zend_update_property_long(reader_tracker_ce, Z_OBJ_P(getThis()), "currentDepth", sizeof("currentDepth") - 1, curDepth);

	if (maxDepth > 0 && curDepth > maxDepth) {
		zend_update_property_long(reader_tracker_ce, Z_OBJ_P(getThis()), "currentDepth", sizeof("currentDepth") - 1, curDepth - 1);
		throw_nbt_data_exception("Nesting level too deep: reached max depth of %ld tags", (long)maxDepth);
		RETURN_THROWS();
	}

	zval retval;
	call_user_function(NULL, NULL, execute, &retval, 0, NULL);
	zval_ptr_dtor(&retval);

	zend_update_property_long(reader_tracker_ce, Z_OBJ_P(getThis()), "currentDepth", sizeof("currentDepth") - 1, curDepth - 1);
}

static const zend_function_entry reader_tracker_methods[] = {
	PHP_ME(pocketmine_nbt_ReaderTracker, __construct, arginfo_tracker_construct, ZEND_ACC_PUBLIC)
	PHP_ME(pocketmine_nbt_ReaderTracker, protectDepth, arginfo_tracker_protect_depth, ZEND_ACC_PUBLIC)
	PHP_FE_END
};

// -------------------------------------------------------------
// NBT
// -------------------------------------------------------------
ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_nbt_create_tag, 0, 3, pocketmine\\nbt\\tag\\Tag, 0)
	ZEND_ARG_TYPE_INFO(0, type, IS_LONG, 0)
	ZEND_ARG_OBJ_INFO(0, reader, pocketmine\\nbt\\NbtStreamReader, 0)
	ZEND_ARG_OBJ_INFO(0, tracker, pocketmine\\nbt\\ReaderTracker, 0)
ZEND_END_ARG_INFO()

void nbt_create_tag_from_type(int type, zval* reader, zval* tracker, zval* return_value) {
	switch (type) {
		case NBT_TAG_BYTE:
			zend_call_method_with_1_params(NULL, byte_tag_ce, NULL, "read", return_value, reader);
			break;
		case NBT_TAG_SHORT:
			zend_call_method_with_1_params(NULL, short_tag_ce, NULL, "read", return_value, reader);
			break;
		case NBT_TAG_INT:
			zend_call_method_with_1_params(NULL, int_tag_ce, NULL, "read", return_value, reader);
			break;
		case NBT_TAG_LONG:
			zend_call_method_with_1_params(NULL, long_tag_ce, NULL, "read", return_value, reader);
			break;
		case NBT_TAG_FLOAT:
			zend_call_method_with_1_params(NULL, float_tag_ce, NULL, "read", return_value, reader);
			break;
		case NBT_TAG_DOUBLE:
			zend_call_method_with_1_params(NULL, double_tag_ce, NULL, "read", return_value, reader);
			break;
		case NBT_TAG_BYTE_ARRAY:
			zend_call_method_with_1_params(NULL, byte_array_tag_ce, NULL, "read", return_value, reader);
			break;
		case NBT_TAG_STRING:
			zend_call_method_with_1_params(NULL, string_tag_ce, NULL, "read", return_value, reader);
			break;
		case NBT_TAG_LIST:
			zend_call_method_with_2_params(NULL, list_tag_ce, NULL, "read", return_value, reader, tracker);
			break;
		case NBT_TAG_COMPOUND:
			zend_call_method_with_2_params(NULL, compound_tag_ce, NULL, "read", return_value, reader, tracker);
			break;
		case NBT_TAG_INT_ARRAY:
			zend_call_method_with_1_params(NULL, int_array_tag_ce, NULL, "read", return_value, reader);
			break;
		default:
			throw_nbt_data_exception("Unknown NBT tag type %d", type);
			break;
	}
}

PHP_METHOD(pocketmine_nbt_NBT, createTag) {
	zend_long type;
	zval* reader;
	zval* tracker;
	ZEND_PARSE_PARAMETERS_START(3, 3)
		Z_PARAM_LONG(type)
		Z_PARAM_OBJECT_OF_CLASS(reader, nbt_stream_reader_ce)
		Z_PARAM_OBJECT_OF_CLASS(tracker, reader_tracker_ce)
	ZEND_PARSE_PARAMETERS_END();

	nbt_create_tag_from_type((int)type, reader, tracker, return_value);
}

static const zend_function_entry nbt_methods[] = {
	PHP_ME(pocketmine_nbt_NBT, createTag, arginfo_nbt_create_tag, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	PHP_FE_END
};

void register_nbt_tag_classes() {
	zend_class_entry ce;

	// Tag (abstract)
	INIT_NS_CLASS_ENTRY(ce, "pocketmine\\nbt\\tag", "Tag", tag_methods);
	tag_ce = register_internal_class_with_flags(&ce, NULL, ZEND_ACC_EXPLICIT_ABSTRACT_CLASS);
	zend_declare_property_bool(tag_ce, "cloning", sizeof("cloning") - 1, 0, ZEND_ACC_PROTECTED);

	// ImmutableTag (abstract)
	INIT_NS_CLASS_ENTRY(ce, "pocketmine\\nbt\\tag", "ImmutableTag", immutable_tag_methods);
	immutable_tag_ce = register_internal_class_with_flags(&ce, tag_ce, ZEND_ACC_EXPLICIT_ABSTRACT_CLASS);

	// Concrete scalar tags
	INIT_NS_CLASS_ENTRY(ce, "pocketmine\\nbt\\tag", "ByteTag", ByteTag_methods);
	byte_tag_ce = register_internal_class_with_flags(&ce, immutable_tag_ce, ZEND_ACC_FINAL);
	zend_declare_property_null(byte_tag_ce, "value", sizeof("value") - 1, ZEND_ACC_PRIVATE);

	INIT_NS_CLASS_ENTRY(ce, "pocketmine\\nbt\\tag", "ShortTag", ShortTag_methods);
	short_tag_ce = register_internal_class_with_flags(&ce, immutable_tag_ce, ZEND_ACC_FINAL);
	zend_declare_property_null(short_tag_ce, "value", sizeof("value") - 1, ZEND_ACC_PRIVATE);

	INIT_NS_CLASS_ENTRY(ce, "pocketmine\\nbt\\tag", "IntTag", IntTag_methods);
	int_tag_ce = register_internal_class_with_flags(&ce, immutable_tag_ce, ZEND_ACC_FINAL);
	zend_declare_property_null(int_tag_ce, "value", sizeof("value") - 1, ZEND_ACC_PRIVATE);

	INIT_NS_CLASS_ENTRY(ce, "pocketmine\\nbt\\tag", "LongTag", LongTag_methods);
	long_tag_ce = register_internal_class_with_flags(&ce, immutable_tag_ce, ZEND_ACC_FINAL);
	zend_declare_property_null(long_tag_ce, "value", sizeof("value") - 1, ZEND_ACC_PRIVATE);

	INIT_NS_CLASS_ENTRY(ce, "pocketmine\\nbt\\tag", "FloatTag", float_tag_methods);
	float_tag_ce = register_internal_class_with_flags(&ce, immutable_tag_ce, ZEND_ACC_FINAL);
	zend_declare_property_null(float_tag_ce, "value", sizeof("value") - 1, ZEND_ACC_PRIVATE);

	INIT_NS_CLASS_ENTRY(ce, "pocketmine\\nbt\\tag", "DoubleTag", double_tag_methods);
	double_tag_ce = register_internal_class_with_flags(&ce, immutable_tag_ce, ZEND_ACC_FINAL);
	zend_declare_property_null(double_tag_ce, "value", sizeof("value") - 1, ZEND_ACC_PRIVATE);

	INIT_NS_CLASS_ENTRY(ce, "pocketmine\\nbt\\tag", "ByteArrayTag", byte_array_tag_methods);
	byte_array_tag_ce = register_internal_class_with_flags(&ce, immutable_tag_ce, ZEND_ACC_FINAL);
	zend_declare_property_null(byte_array_tag_ce, "value", sizeof("value") - 1, ZEND_ACC_PRIVATE);

	INIT_NS_CLASS_ENTRY(ce, "pocketmine\\nbt\\tag", "StringTag", string_tag_methods);
	string_tag_ce = register_internal_class_with_flags(&ce, immutable_tag_ce, ZEND_ACC_FINAL);
	zend_declare_property_null(string_tag_ce, "value", sizeof("value") - 1, ZEND_ACC_PRIVATE);

	INIT_NS_CLASS_ENTRY(ce, "pocketmine\\nbt\\tag", "IntArrayTag", int_array_tag_methods);
	int_array_tag_ce = register_internal_class_with_flags(&ce, immutable_tag_ce, ZEND_ACC_FINAL);
	zend_declare_property_null(int_array_tag_ce, "value", sizeof("value") - 1, ZEND_ACC_PRIVATE);

	// ListTag
	INIT_NS_CLASS_ENTRY(ce, "pocketmine\\nbt\\tag", "ListTag", list_tag_methods);
	list_tag_ce = register_internal_class_with_flags(&ce, tag_ce, ZEND_ACC_FINAL);
	zend_class_implements(list_tag_ce, 2, zend_ce_countable, zend_ce_aggregate);
	zend_declare_property_long(list_tag_ce, "tagType", sizeof("tagType") - 1, 0, ZEND_ACC_PRIVATE);
	zend_declare_property_null(list_tag_ce, "value", sizeof("value") - 1, ZEND_ACC_PRIVATE);

	// CompoundTag
	INIT_NS_CLASS_ENTRY(ce, "pocketmine\\nbt\\tag", "CompoundTag", compound_tag_methods);
	compound_tag_ce = register_internal_class_with_flags(&ce, tag_ce, ZEND_ACC_FINAL);
	zend_class_implements(compound_tag_ce, 2, zend_ce_countable, zend_ce_aggregate);
	zend_declare_property_null(compound_tag_ce, "value", sizeof("value") - 1, ZEND_ACC_PRIVATE);
}

void register_nbt_root_classes() {
	zend_class_entry ce;

	// TreeRoot
	INIT_NS_CLASS_ENTRY(ce, "pocketmine\\nbt", "TreeRoot", tree_root_methods);
	tree_root_ce = zend_register_internal_class(&ce);
	zend_declare_property_null(tree_root_ce, "root", sizeof("root") - 1, ZEND_ACC_PRIVATE);
	zend_declare_property_string(tree_root_ce, "name", sizeof("name") - 1, "", ZEND_ACC_PRIVATE);

	// ReaderTracker
	INIT_NS_CLASS_ENTRY(ce, "pocketmine\\nbt", "ReaderTracker", reader_tracker_methods);
	reader_tracker_ce = zend_register_internal_class(&ce);
	zend_declare_property_null(reader_tracker_ce, "maxDepth", sizeof("maxDepth") - 1, ZEND_ACC_PRIVATE);
	zend_declare_property_long(reader_tracker_ce, "currentDepth", sizeof("currentDepth") - 1, 0, ZEND_ACC_PRIVATE);

	// NBT
	INIT_NS_CLASS_ENTRY(ce, "pocketmine\\nbt", "NBT", nbt_methods);
	nbt_ce = register_internal_class_with_flags(&ce, NULL, ZEND_ACC_EXPLICIT_ABSTRACT_CLASS);
	zend_declare_class_constant_long(nbt_ce, "TAG_End", sizeof("TAG_End") - 1, NBT_TAG_END);
	zend_declare_class_constant_long(nbt_ce, "TAG_Byte", sizeof("TAG_Byte") - 1, NBT_TAG_BYTE);
	zend_declare_class_constant_long(nbt_ce, "TAG_Short", sizeof("TAG_Short") - 1, NBT_TAG_SHORT);
	zend_declare_class_constant_long(nbt_ce, "TAG_Int", sizeof("TAG_Int") - 1, NBT_TAG_INT);
	zend_declare_class_constant_long(nbt_ce, "TAG_Long", sizeof("TAG_Long") - 1, NBT_TAG_LONG);
	zend_declare_class_constant_long(nbt_ce, "TAG_Float", sizeof("TAG_Float") - 1, NBT_TAG_FLOAT);
	zend_declare_class_constant_long(nbt_ce, "TAG_Double", sizeof("TAG_Double") - 1, NBT_TAG_DOUBLE);
	zend_declare_class_constant_long(nbt_ce, "TAG_ByteArray", sizeof("TAG_ByteArray") - 1, NBT_TAG_BYTE_ARRAY);
	zend_declare_class_constant_long(nbt_ce, "TAG_String", sizeof("TAG_String") - 1, NBT_TAG_STRING);
	zend_declare_class_constant_long(nbt_ce, "TAG_List", sizeof("TAG_List") - 1, NBT_TAG_LIST);
	zend_declare_class_constant_long(nbt_ce, "TAG_Compound", sizeof("TAG_Compound") - 1, NBT_TAG_COMPOUND);
	zend_declare_class_constant_long(nbt_ce, "TAG_IntArray", sizeof("TAG_IntArray") - 1, NBT_TAG_INT_ARRAY);
}
