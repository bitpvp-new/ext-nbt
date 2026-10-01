--TEST--
Check for nbt extension loading and constants
--SKIPIF--
<?php if (!extension_loaded("nbt")) die("skip nbt extension not loaded"); ?>
--FILE--
<?php
echo "nbt extension is loaded\n";
var_dump(pocketmine\nbt\NBT::TAG_End);
var_dump(pocketmine\nbt\NBT::TAG_Byte);
var_dump(pocketmine\nbt\NBT::TAG_Short);
var_dump(pocketmine\nbt\NBT::TAG_Int);
var_dump(pocketmine\nbt\NBT::TAG_Long);
var_dump(pocketmine\nbt\NBT::TAG_Float);
var_dump(pocketmine\nbt\NBT::TAG_Double);
var_dump(pocketmine\nbt\NBT::TAG_ByteArray);
var_dump(pocketmine\nbt\NBT::TAG_String);
var_dump(pocketmine\nbt\NBT::TAG_List);
var_dump(pocketmine\nbt\NBT::TAG_Compound);
var_dump(pocketmine\nbt\NBT::TAG_IntArray);
?>
--EXPECT--
nbt extension is loaded
int(0)
int(1)
int(2)
int(3)
int(4)
int(5)
int(6)
int(7)
int(8)
int(9)
int(10)
int(11)
