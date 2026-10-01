--TEST--
NoDynamicFieldsTrait prevents undeclared dynamic properties
--SKIPIF--
<?php if (!extension_loaded("nbt")) print "skip"; ?>
--FILE--
<?php
use pocketmine\nbt\tag\CompoundTag;
use pocketmine\nbt\tag\ListTag;

$tag = new ListTag();
try {
	$tag->foo = "bar";
	echo "FAIL\n";
} catch (\Throwable $e) {
	echo "CAUGHT DYNAMIC PROPERTY ERROR\n";
}

$compound = new CompoundTag();
try {
	$compound->dynamicProp = 123;
	echo "FAIL\n";
} catch (\Throwable $e) {
	echo "CAUGHT COMPOUND DYNAMIC PROPERTY ERROR\n";
}

echo "DONE\n";
?>
--EXPECT--
CAUGHT DYNAMIC PROPERTY ERROR
CAUGHT COMPOUND DYNAMIC PROPERTY ERROR
DONE
