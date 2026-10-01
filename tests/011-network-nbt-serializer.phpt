--TEST--
NetworkNbtSerializer write and read with Bedrock network protocol format
--SKIPIF--
<?php if (!extension_loaded("nbt")) print "skip"; ?>
--FILE--
<?php
use pocketmine\network\mcpe\protocol\serializer\NetworkNbtSerializer;
use pocketmine\nbt\TreeRoot;
use pocketmine\nbt\tag\CompoundTag;
use pocketmine\nbt\tag\IntTag;
use pocketmine\nbt\tag\ListTag;

var_dump(class_exists(NetworkNbtSerializer::class));
var_dump(is_subclass_of(NetworkNbtSerializer::class, \pocketmine\nbt\BaseNbtSerializer::class));

$serializer = new NetworkNbtSerializer();

$compound = CompoundTag::create()
	->setByte("byte", 100)
	->setShort("short", 20000)
	->setInt("int", 1234567)
	->setLong("long", 9876543210123)
	->setString("str", "Bedrock network NBT")
	->setByteArray("bytes", "binary_data")
	->setIntArray("ints", [1, -2, 300, -4000])
	->setTag("nested", CompoundTag::create()->setInt("child_int", 999))
	->setTag("list", new ListTag([new IntTag(5), new IntTag(10)]));

$root = new TreeRoot($compound, "");
$encoded = $serializer->write($root);

$offset = 0;
$decodedRoot = $serializer->read($encoded, $offset);
var_dump($offset === strlen($encoded));
var_dump($decodedRoot->getName() === "");

$decoded = $decodedRoot->mustGetCompoundTag();
var_dump($decoded->getByte("byte") === 100);
var_dump($decoded->getShort("short") === 20000);
var_dump($decoded->getInt("int") === 1234567);
var_dump($decoded->getLong("long") === 9876543210123);
var_dump($decoded->getString("str") === "Bedrock network NBT");
var_dump($decoded->getByteArray("bytes") === "binary_data");
var_dump($decoded->getIntArray("ints") === [1, -2, 300, -4000]);
var_dump($decoded->getCompoundTag("nested")->getInt("child_int") === 999);
var_dump($decoded->getListTag("list")->count() === 2);

// Headless test
$headless = $serializer->writeHeadless($compound);
$decodedHeadless = $serializer->readHeadless($headless, \pocketmine\nbt\NBT::TAG_Compound);
var_dump($decodedHeadless->equals($compound));

echo "DONE\n";
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
DONE
