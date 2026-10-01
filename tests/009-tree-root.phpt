--TEST--
TreeRoot tag wrapper and validations
--SKIPIF--
<?php if (!extension_loaded("nbt")) print "skip"; ?>
--FILE--
<?php
use pocketmine\nbt\TreeRoot;
use pocketmine\nbt\tag\CompoundTag;
use pocketmine\nbt\tag\IntTag;

$compound = new CompoundTag();
$root = new TreeRoot($compound, "hello");

var_dump($root->getTag() === $compound);
var_dump($root->mustGetCompoundTag() === $compound);
var_dump($root->getName() === "hello");

$root2 = new TreeRoot(clone $compound, "hello");
var_dump($root->equals($root2));

$intRoot = new TreeRoot(new IntTag(10), "int");
try {
	$intRoot->mustGetCompoundTag();
	echo "FAIL\n";
} catch (\pocketmine\nbt\UnexpectedTagTypeException $e) {
	echo "CAUGHT UNEXPECTED TAG TYPE\n";
}

// Name too long check
try {
	new TreeRoot($compound, str_repeat("a", 32768));
	echo "FAIL\n";
} catch (\InvalidArgumentException $e) {
	echo "CAUGHT NAME TOO LONG\n";
}

echo "DONE\n";
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
bool(true)
CAUGHT UNEXPECTED TAG TYPE
CAUGHT NAME TOO LONG
DONE
