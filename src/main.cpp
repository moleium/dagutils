#include "bitstream.h"
#include <EASTL/string.h>
#include <EASTL/vector.h>
#include <stdio.h>

struct CatData {
  uint16_t catId;
  eastl::string catName;
  uint8_t ageYears;
  bool isSleeping;
};

bool parseCatData(danet::BitStream &stream, CatData &cat) {
  if (!stream.ReadCompressed(cat.catId))
    return false;
  if (!stream.Read(cat.catName))
    return false;
  if (!stream.Read(cat.ageYears))
    return false;
  if (!stream.Read(cat.isSleeping))
    return false;

  return true;
}

int main() {
  danet::BitStream writeStream;

  writeStream.WriteCompressed(uint16_t(42));        // catId = 42
  writeStream.Write(eastl::string("Whiskers")); // catName = "Whiskers"
  writeStream.Write(uint8_t(3));                    // ageYears = 3
  writeStream.Write(true);                          // isSleeping = true

  const uint8_t *data = writeStream.GetData();
  size_t size = writeStream.GetNumberOfBytesUsed();

  danet::BitStream readStream(data, size, false);

  CatData parsedCat;
  if (parseCatData(readStream, parsedCat)) {
    printf("Parsed Cat:\n");
    printf("  ID: %u\n", parsedCat.catId);
    printf("  Name: %s\n", parsedCat.catName.c_str());
    printf("  Age (Years): %u\n", parsedCat.ageYears);
    printf("  Is Sleeping: %s\n", parsedCat.isSleeping ? "true" : "false");
  } else {
    printf("Failed to parse cat data.\n");
  }

  printf("Remaining bits in stream: %u\n", readStream.GetNumberOfUnreadBits());

  return 0;
}