--TEST--
Test ListTag operations
--SKIPIF--
<?php if (!extension_loaded("nbt")) die("skip nbt extension not loaded"); ?>
--FILE--
<?php
use pocketmine\nbt\tag\ListTag;
use pocketmine\nbt\tag\IntTag;
use pocketmine\nbt\tag\StringTag;
use pocketmine\nbt\NBT;

$list = new ListTag();
$list->push(new IntTag(10));
$list->push(new IntTag(20));
$list->push(new IntTag(30));

var_dump(count($list));
var_dump($list->getTagType() === NBT::TAG_Int);
var_dump($list->getAllValues());
var_dump($list->first()->getValue());
var_dump($list->last()->getValue());

$list->insert(1, new IntTag(15));
var_dump($list->getAllValues());

$list->remove(2);
var_dump($list->getAllValues());

try {
    $list->push(new StringTag("error"));
} catch (TypeError $e) {
    echo "TypeError caught\n";
}

$popped = $list->pop();
var_dump($popped->getValue());

$shifted = $list->shift();
var_dump($shifted->getValue());

$list->unshift(new IntTag(5));
var_dump($list->first()->getValue());
?>
--EXPECT--
int(3)
bool(true)
array(3) {
  [0]=>
  int(10)
  [1]=>
  int(20)
  [2]=>
  int(30)
}
int(10)
int(30)
array(4) {
  [0]=>
  int(10)
  [1]=>
  int(15)
  [2]=>
  int(20)
  [3]=>
  int(30)
}
array(3) {
  [0]=>
  int(10)
  [1]=>
  int(15)
  [2]=>
  int(30)
}
TypeError caught
int(30)
int(10)
int(5)
