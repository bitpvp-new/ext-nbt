/* nbt extension for PHP */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "php_nbt.h"
#include "ZendUtil.h"
#include "NbtTags.h"
#include "NbtSerializer.h"
#include "JsonNbtParser.h"

/* {{{ PHP_MINFO_FUNCTION */
PHP_MINFO_FUNCTION(nbt)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "nbt support", "enabled");
	php_info_print_table_row(2, "Version", PHP_NBT_VERSION);
	php_info_print_table_row(2, "Drop-in replacement for", "pocketmine/nbt");
	php_info_print_table_end();
}
/* }}} */

/* {{{ PHP_RINIT_FUNCTION */
PHP_RINIT_FUNCTION(nbt)
{
#if defined(ZTS) && defined(COMPILE_DL_NBT)
	ZEND_TSRMLS_CACHE_UPDATE();
#endif
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_RSHUTDOWN_FUNCTION */
PHP_RSHUTDOWN_FUNCTION(nbt)
{
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINIT_FUNCTION */
PHP_MINIT_FUNCTION(nbt)
{
	register_nbt_exceptions();
	register_nbt_interfaces_traits();
	register_nbt_tag_classes();
	register_nbt_root_classes();
	register_nbt_serializer_classes();
	register_json_nbt_parser_class();

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION */
PHP_MSHUTDOWN_FUNCTION(nbt)
{
	return SUCCESS;
}
/* }}} */

static const zend_module_dep module_dependencies[] = {
	ZEND_MOD_REQUIRED("spl")
	ZEND_MOD_END
};

/* {{{ nbt_module_entry */
zend_module_entry nbt_module_entry = {
	STANDARD_MODULE_HEADER_EX,
	NULL, /* ini_entries */
	module_dependencies,
	"nbt",					/* Extension name */
	NULL,					/* zend_function_entry */
	PHP_MINIT(nbt),			/* PHP_MINIT - Module initialization */
	PHP_MSHUTDOWN(nbt),		/* PHP_MSHUTDOWN - Module shutdown */
	PHP_RINIT(nbt),			/* PHP_RINIT - Request initialization */
	PHP_RSHUTDOWN(nbt),		/* PHP_RSHUTDOWN - Request shutdown */
	PHP_MINFO(nbt),			/* PHP_MINFO - Module info */
	PHP_NBT_VERSION,		/* Version */
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_NBT
# ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE()
# endif
ZEND_GET_MODULE(nbt)
#endif
