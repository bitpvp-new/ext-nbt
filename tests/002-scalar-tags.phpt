--TEST--
Test scalar NBT tags
--SKIPIF--
<?php if (!extension_loaded("nbt")) die("skip nbt extension not loaded"); ?>
--FILE--
<?php
use pocketmine\nbt\tag\ByteTag;
use pocketmine\nbt\tag\ShortTag;
use pocketmine\nbt\tag\IntTag;
use pocketmine\nbt\tag\LongTag;
use pocketmine\nbt\tag\FloatTag;
use pocketmine\nbt\tag\DoubleTag;
use pocketmine\nbt\tag\ByteArrayTag;
use pocketmine\nbt\tag\StringTag;
use pocketmine\nbt\tag\IntArrayTag;
use pocketmine\nbt\InvalidTagValueException;

$b = new ByteTag(127);
var_dump($b->getValue(), $b->getType());

try {
    new ByteTag(128);
} catch (InvalidTagValueException $e) {
    echo "ByteTag too large caught\n";
}

try {
    new ByteTag(-129);
} catch (InvalidTagValueException $e) {
    echo "ByteTag too small caught\n";
}

$s = new ShortTag(32767);
var_dump($s->getValue(), $s->getType());

$i = new IntTag(2147483647);
var_dump($i->getValue(), $i->getType());

$l = new LongTag(9223372036854775807);
var_dump($l->getValue(), $l->getType());

$f = new FloatTag(1.5);
var_dump($f->getValue(), $f->getType());

$f1 = new FloatTag(0.3);
$f2 = new FloatTag(0.3);
var_dump($f1->equals($f2));

$d = new DoubleTag(2.5);
var_dump($d->getValue(), $d->getType());

$ba = new ByteArrayTag("hello");
var_dump($ba->getValue(), $ba->getType(), (string)$ba);

$str = new StringTag("world");
var_dump($str->getValue(), $str->getType(), (string)$str);

try {
    new StringTag(str_repeat("a", 32768));
} catch (InvalidTagValueException $e) {
    echo "StringTag too long caught\n";
}

$ia = new IntArrayTag([1, 2, 3]);
var_dump($ia->getValue(), $ia->getType(), (string)$ia);
?>
--EXPECT--
int(127)
int(1)
ByteTag too large caught
ByteTag too small caught
int(32767)
int(2)
int(2147483647)
int(3)
int(9223372036854775807)
int(4)
float(1.5)
int(5)
bool(true)
float(2.5)
int(6)
string(5) "hello"
int(7)
string(26) "TAG_ByteArray=b64:aGVsbG8="
string(5) "world"
int(8)
string(18) "TAG_String="world""
StringTag too long caught
array(3) {
  [0]=>
  int(1)
  [1]=>
  int(2)
  [2]=>
  int(3)
}
int(11)
string(20) "TAG_IntArray=[1,2,3]"
