--TEST--
JsonNbtParser parsing JSON into CompoundTag
--SKIPIF--
<?php if (!extension_loaded("nbt")) print "skip"; ?>
--FILE--
<?php
use pocketmine\nbt\JsonNbtParser;
use pocketmine\nbt\tag\ByteTag;
use pocketmine\nbt\tag\ShortTag;
use pocketmine\nbt\tag\IntTag;
use pocketmine\nbt\tag\LongTag;
use pocketmine\nbt\tag\FloatTag;
use pocketmine\nbt\tag\DoubleTag;
use pocketmine\nbt\tag\StringTag;

$json = '{
	"byte": 1b,
	"short": 2s,
	"int": 3,
	"long": 4l,
	"float": 5.5f,
	"double": 6.6d,
	"string": "test",
	"unquoted_string": unquoted,
	"nested": {
		"foo": "bar"
	},
	"list": [1, 2, 3]
}';

$tag = JsonNbtParser::parseJson($json);
var_dump($tag->getTag("byte") instanceof ByteTag && $tag->getByte("byte") === 1);
var_dump($tag->getTag("short") instanceof ShortTag && $tag->getShort("short") === 2);
var_dump($tag->getTag("int") instanceof IntTag && $tag->getInt("int") === 3);
var_dump($tag->getTag("long") instanceof LongTag && $tag->getLong("long") === 4);
var_dump($tag->getTag("float") instanceof FloatTag);
var_dump($tag->getTag("double") instanceof DoubleTag);
var_dump($tag->getString("string") === "test");
var_dump($tag->getString("unquoted_string") === "unquoted");
var_dump($tag->getCompoundTag("nested")->getString("foo") === "bar");
var_dump($tag->getListTag("list")->count() === 3);

// Syntax error check
try {
	JsonNbtParser::parseJson('{ bad json');
	echo "FAIL\n";
} catch (\pocketmine\nbt\NbtDataException $e) {
	echo "CAUGHT SYNTAX ERROR\n";
}

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
CAUGHT SYNTAX ERROR
DONE
