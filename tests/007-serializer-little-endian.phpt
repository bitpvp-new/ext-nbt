--TEST--
LittleEndianNbtSerializer write and read
--SKIPIF--
<?php if (!extension_loaded("nbt")) print "skip"; ?>
--FILE--
<?php
use pocketmine\nbt\LittleEndianNbtSerializer;
use pocketmine\nbt\TreeRoot;
use pocketmine\nbt\tag\CompoundTag;
use pocketmine\nbt\tag\IntTag;
use pocketmine\nbt\tag\ListTag;

$serializer = new LittleEndianNbtSerializer();

$compound = CompoundTag::create()
	->setByte("byte", 5)
	->setShort("short", 500)
	->setInt("int", 50000)
	->setString("str", "pocketmine")
	->setIntArray("ints", [10, 20, 30])
	->setTag("list", new ListTag([new IntTag(1), new IntTag(2)]));

$root = new TreeRoot($compound, "");
$encoded = $serializer->write($root);

$offset = 0;
$decodedRoot = $serializer->read($encoded, $offset);
var_dump($offset === strlen($encoded));
var_dump($decodedRoot->getName() === "");

$decoded = $decodedRoot->mustGetCompoundTag();
var_dump($decoded->getInt("int") === 50000);
var_dump($decoded->getString("str") === "pocketmine");
var_dump($decoded->getIntArray("ints") === [10, 20, 30]);

echo "DONE\n";
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
DONE
