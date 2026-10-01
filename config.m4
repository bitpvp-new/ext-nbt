PHP_ARG_ENABLE([nbt],
  [whether to enable Named Binary Tag support],
  [AS_HELP_STRING([--enable-nbt],
    [Enable Named Binary Tag support])],
  [no])

if test "$PHP_NBT" != "no"; then
  PHP_NEW_EXTENSION(nbt,
    nbt.c \
    NbtTags.c \
    NbtSerializer.c \
    JsonNbtParser.c,
    $ext_shared,
    -DZEND_ENABLE_STATIC_TSRMLS_CACHE=1 -std=c99 -Wall -Wno-unused-function)
  PHP_SUBST(NBT_SHARED_LIBADD)
  PHP_ADD_INCLUDE($ext_srcdir)
  PHP_ADD_INCLUDE($ext_builddir)
fi
