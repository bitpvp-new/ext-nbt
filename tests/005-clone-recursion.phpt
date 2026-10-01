--TEST--
Clone recursion detection on CompoundTag and ListTag
--SKIPIF--
<?php if (!extension_loaded("nbt")) print "skip"; ?>
--FILE--
<?php
use pocketmine\nbt\tag\CompoundTag;
use pocketmine\nbt\tag\ListTag;
use pocketmine\nbt\tag\IntTag;

$root = new CompoundTag();
$child = new CompoundTag();
$child->setTag("int", new IntTag(42));
$root->setTag("child", $child);

$cloned = clone $root;
var_dump($cloned !== $root);
var_dump($cloned->getCompoundTag("child") !== $child);
var_dump($cloned->getCompoundTag("child")->getInt("int") === 42);

$list = new ListTag([new CompoundTag()]);
$clonedList = clone $list;
var_dump($clonedList !== $list);
var_dump($clonedList->get(0) !== $list->get(0));
echo "DONE\n";
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
DONE
