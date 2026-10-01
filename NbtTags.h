#ifndef NBT_TAGS_H
#define NBT_TAGS_H

#include "ZendUtil.h"

#define NBT_TAG_END 0
#define NBT_TAG_BYTE 1
#define NBT_TAG_SHORT 2
#define NBT_TAG_INT 3
#define NBT_TAG_LONG 4
#define NBT_TAG_FLOAT 5
#define NBT_TAG_DOUBLE 6
#define NBT_TAG_BYTE_ARRAY 7
#define NBT_TAG_STRING 8
#define NBT_TAG_LIST 9
#define NBT_TAG_COMPOUND 10
#define NBT_TAG_INT_ARRAY 11

void register_nbt_exceptions();
void register_nbt_interfaces_traits();
void register_nbt_tag_classes();
void register_nbt_root_classes();

/* Shared tag construction helper */
void nbt_create_tag_from_type(int type, zval* reader_zv, zval* tracker_zv, zval* return_value);

#endif /* NBT_TAGS_H */
