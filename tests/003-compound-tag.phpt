--TEST--
Test CompoundTag operations
--SKIPIF--
<?php if (!extension_loaded("nbt")) die("skip nbt extension not loaded"); ?>
--FILE--
<?php
use pocketmine\nbt\tag\CompoundTag;
use pocketmine\nbt\tag\StringTag;
use pocketmine\nbt\tag\IntTag;
use pocketmine\nbt\tag\ListTag;
use pocketmine\nbt\UnexpectedTagTypeException;
use pocketmine\nbt\NoSuchTagException;

$c = CompoundTag::create()
    ->setString("name", "steve")
    ->setInt("age", 20)
    ->setFloat("health", 20.0);

var_dump(count($c));
var_dump($c->getString("name"));
var_dump($c->getInt("age"));
var_dump($c->getFloat("health"));
var_dump($c->getInt("nonexistent", 99));

try {
    $c->getInt("nonexistent");
} catch (NoSuchTagException $e) {
    echo "NoSuchTagException caught\n";
}

try {
    $c->getInt("name");
} catch (UnexpectedTagTypeException $e) {
    echo "UnexpectedTagTypeException caught\n";
}

$c->removeTag("health");
var_dump(count($c));

$c2 = CompoundTag::create()
    ->setString("name", "alex")
    ->setInt("score", 100);

$merged = $c->merge($c2);
var_dump($merged->getString("name"));
var_dump($merged->getInt("age"));
var_dump($merged->getInt("score"));

$c3 = clone $c;
var_dump($c->equals($c3));
?>
--EXPECT--
int(3)
string(5) "steve"
int(20)
float(20)
int(99)
NoSuchTagException caught
UnexpectedTagTypeException caught
int(2)
string(4) "alex"
int(20)
int(100)
bool(true)
