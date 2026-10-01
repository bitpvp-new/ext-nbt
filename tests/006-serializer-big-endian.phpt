--TEST--
BigEndianNbtSerializer write and read
--SKIPIF--
<?php if (!extension_loaded("nbt")) print "skip"; ?>
--FILE--
<?php
use pocketmine\nbt\BigEndianNbtSerializer;
use pocketmine\nbt\TreeRoot;
use pocketmine\nbt\tag\ByteTag;
use pocketmine\nbt\tag\ShortTag;
use pocketmine\nbt\tag\IntTag;
use pocketmine\nbt\tag\LongTag;
use pocketmine\nbt\tag\FloatTag;
use pocketmine\nbt\tag\DoubleTag;
use pocketmine\nbt\tag\StringTag;
use pocketmine\nbt\tag\ByteArrayTag;
use pocketmine\nbt\tag\IntArrayTag;
use pocketmine\nbt\tag\CompoundTag;
use pocketmine\nbt\tag\ListTag;

$serializer = new BigEndianNbtSerializer();

$compound = CompoundTag::create()
	->setByte("byte", 12)
	->setShort("short", 1234)
	->setInt("int", 123456)
	->setLong("long", 1234567890123)
	->setString("string", "Hello world")
	->setByteArray("bytes", "\x01\x02\x03\x04")
	->setIntArray("ints", [1, 2, 3])
	->setTag("list", new ListTag([new IntTag(10), new IntTag(20)]));

$root = new TreeRoot($compound, "TestRoot");
$encoded = $serializer->write($root);

$offset = 0;
$decodedRoot = $serializer->read($encoded, $offset);
var_dump($offset === strlen($encoded));
var_dump($decodedRoot->getName() === "TestRoot");

$decoded = $decodedRoot->mustGetCompoundTag();
var_dump($decoded->getByte("byte") === 12);
var_dump($decoded->getShort("short") === 1234);
var_dump($decoded->getInt("int") === 123456);
var_dump($decoded->getLong("long") === 1234567890123);
var_dump($decoded->getString("string") === "Hello world");
var_dump($decoded->getByteArray("bytes") === "\x01\x02\x03\x04");
var_dump($decoded->getIntArray("ints") === [1, 2, 3]);

// Headless test
$headlessEncoded = $serializer->writeHeadless($compound);
$decodedHeadless = $serializer->readHeadless($headlessEncoded, \pocketmine\nbt\NBT::TAG_Compound);
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
DONE
