/* nbt extension for PHP */

#ifndef PHP_NBT_H
# define PHP_NBT_H

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "php.h"
extern zend_module_entry nbt_module_entry;
#define phpext_nbt_ptr &nbt_module_entry

# define PHP_NBT_VERSION "1.0.0"

# if defined(ZTS) && defined(COMPILE_DL_NBT)
ZEND_TSRMLS_CACHE_EXTERN()
# endif

#endif	/* PHP_NBT_H */
