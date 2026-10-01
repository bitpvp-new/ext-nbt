# ext-nbt
![CI](https://github.com/pmmp/NBT/workflows/CI/badge.svg)

High-performance PHP extension written in C for working with the NBT (Named Binary Tag) data storage format.
100% drop-in replacement for `pocketmine/nbt` with no backward-compatibility breaks.

## Features
- **Zero BC breaks**: Exact drop-in replacement for the original pure-PHP `pocketmine/nbt` library (classes, methods, constants, hierarchy, exception types and messages).
- **Extreme Performance**: Tag operations, cloning, binary encoding/decoding, and JSON-NBT parsing implemented natively in C.
- **Full Serialization Support**:
  - `pocketmine\nbt\BigEndianNbtSerializer`: Standard Minecraft Java edition format.
  - `pocketmine\nbt\LittleEndianNbtSerializer`: Minecraft Bedrock edition disk storage format.
  - `pocketmine\network\mcpe\protocol\serializer\NetworkNbtSerializer`: Minecraft Bedrock edition network protocol format (VarInt / VarLong / Little Endian).

## Installation

### Linux / macOS
```bash
phpize
./configure
make -j$(nproc)
sudo make install
```
Add the extension to your `php.ini`:
```ini
extension=nbt.so
```

### Windows
Compile using MSVC with the PHP SDK:
```cmd
phpize
configure --enable-nbt
nmake
```
Add to `php.ini`:
```ini
extension=php_nbt.dll
```

## Running Tests
Run the PHPT test suite using:
```bash
make test TESTS="-q --show-diff tests/"
```

## Examples

### Reading data
```php
use pocketmine\nbt\LittleEndianNbtSerializer;
use pocketmine\nbt\NbtDataException;
use pocketmine\nbt\NoSuchTagException;
use pocketmine\nbt\UnexpectedTagTypeException;
use pocketmine\nbt\tag\StringTag;

$serializer = new LittleEndianNbtSerializer();
$optionalStartOffset = 0;
$optionalMaxDepth = 0; // unlimited by default
$treeRoot = $serializer->read($yourInputBytes, $optionalStartOffset, $optionalMaxDepth);

try{
    // If you expect a TAG_Compound root (the most common case)
    $data = $treeRoot->mustGetCompoundTag();
}catch(NbtDataException $e){
    var_dump("root isn't a TAG_Compound");
}

// For other cases where root is not a compound
var_dump($treeRoot->getTag());
var_dump($treeRoot->getName());

$str = $data->getString("hello", "default");
$nestedCompound = $data->getCompoundTag("nestedCompound");
$nestedList = $data->getListTag("listOfStrings", StringTag::class);
```

### Writing data
```php
use pocketmine\nbt\LittleEndianNbtSerializer;
use pocketmine\nbt\TreeRoot;
use pocketmine\nbt\tag\CompoundTag;
use pocketmine\nbt\tag\IntTag;
use pocketmine\nbt\tag\ListTag;
use pocketmine\nbt\tag\StringTag;

$compound = CompoundTag::create()
    ->setByte("byte", 1)
    ->setInt("int", 2)
    ->setTag("list", new ListTag([
        new StringTag("item1"),
        new StringTag("item2")
    ]))
    ->setTag("compound", CompoundTag::create()
        ->setByte("nestedByte", 1)
    );

$serializer = new LittleEndianNbtSerializer();
$bytes = $serializer->write(new TreeRoot($compound, "rootName"));
```

### Bedrock Network NBT
```php
use pocketmine\network\mcpe\protocol\serializer\NetworkNbtSerializer;
use pocketmine\nbt\TreeRoot;
use pocketmine\nbt\tag\CompoundTag;

$networkSerializer = new NetworkNbtSerializer();
$bytes = $networkSerializer->write(new TreeRoot($compound, ""));
$decodedRoot = $networkSerializer->read($bytes);
```

## License
Licensed under LGPL-3.0-or-later.
