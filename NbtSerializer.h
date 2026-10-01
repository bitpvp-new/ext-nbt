#ifndef NBT_SERIALIZER_H
#define NBT_SERIALIZER_H

#include "ZendUtil.h"

extern zend_class_entry* base_nbt_serializer_ce;
extern zend_class_entry* big_endian_nbt_serializer_ce;
extern zend_class_entry* little_endian_nbt_serializer_ce;
extern zend_class_entry* network_nbt_serializer_ce;

void register_nbt_serializer_classes();

#endif /* NBT_SERIALIZER_H */
