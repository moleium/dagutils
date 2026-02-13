
#include <emscripten/bind.h>
#include <emscripten/val.h>

#include <bitstream/bitstream.h>
#include <datablock/datablock.h>
#include <memory/dag_mem.h>

using namespace emscripten;

EMSCRIPTEN_BINDINGS(dagutils) {
  class_<danet::BitStream>("BitStream")
      .constructor<>()
      .function("GetNumberOfBitsUsed", &danet::BitStream::GetNumberOfBitsUsed)
      .function("GetNumberOfBytesUsed", &danet::BitStream::GetNumberOfBytesUsed)
      .function("GetReadOffset", &danet::BitStream::GetReadOffset)
      .function("SetReadOffset", &danet::BitStream::SetReadOffset)
      .function("GetNumberOfUnreadBits",
                &danet::BitStream::GetNumberOfUnreadBits)
      .function("Reset", &danet::BitStream::Reset)
      .function("Clear", &danet::BitStream::Clear)
      .function("AlignWriteToByteBoundary",
                &danet::BitStream::AlignWriteToByteBoundary)
      .function("AlignReadToByteBoundary",
                &danet::BitStream::AlignReadToByteBoundary)
      .function("WriteRaw", optional_override([](danet::BitStream &self,
                                                 const std::string &data) {
                  self.Write(data.c_str(), (uint32_t)data.length());
                }))

      .function("WriteInt8",
                optional_override([](danet::BitStream &self, int8_t val) {
                  self.Write<int8_t>(val);
                }))
      .function("WriteUInt8",
                optional_override([](danet::BitStream &self, uint8_t val) {
                  self.Write<uint8_t>(val);
                }))
      .function("WriteInt16",
                optional_override([](danet::BitStream &self, int16_t val) {
                  self.Write<int16_t>(val);
                }))
      .function("WriteUInt16",
                optional_override([](danet::BitStream &self, uint16_t val) {
                  self.Write<uint16_t>(val);
                }))
      .function("WriteInt32",
                optional_override([](danet::BitStream &self, int32_t val) {
                  self.Write<int32_t>(val);
                }))
      .function("WriteUInt32",
                optional_override([](danet::BitStream &self, uint32_t val) {
                  self.Write<uint32_t>(val);
                }))
      .function("WriteInt64",
                optional_override([](danet::BitStream &self, int64_t val) {
                  self.Write<int64_t>(val);
                }))
      .function("WriteUInt64",
                optional_override([](danet::BitStream &self, uint64_t val) {
                  self.Write<uint64_t>(val);
                }))
      .function("WriteFloat",
                optional_override([](danet::BitStream &self, float val) {
                  self.Write<float>(val);
                }))
      .function("WriteDouble",
                optional_override([](danet::BitStream &self, double val) {
                  self.Write<double>(val);
                }))
      .function("WriteBool",
                optional_override([](danet::BitStream &self, bool val) {
                  self.Write<bool>(val);
                }))
      .function("WriteString", optional_override([](danet::BitStream &self,
                                                    const std::string &val) {
                  self.Write<std::string>(val);
                }))

      .function("ReadInt8",
                optional_override([](const danet::BitStream &self) -> val {
                  int8_t v;
                  if (self.Read<int8_t>(v))
                    return val(v);
                  return val::null();
                }))
      .function("ReadUInt8",
                optional_override([](const danet::BitStream &self) -> val {
                  uint8_t v;
                  if (self.Read<uint8_t>(v))
                    return val(v);
                  return val::null();
                }))
      .function("ReadInt16",
                optional_override([](const danet::BitStream &self) -> val {
                  int16_t v;
                  if (self.Read<int16_t>(v))
                    return val(v);
                  return val::null();
                }))
      .function("ReadUInt16",
                optional_override([](const danet::BitStream &self) -> val {
                  uint16_t v;
                  if (self.Read<uint16_t>(v))
                    return val(v);
                  return val::null();
                }))
      .function("ReadInt32",
                optional_override([](const danet::BitStream &self) -> val {
                  int32_t v;
                  if (self.Read<int32_t>(v))
                    return val(v);
                  return val::null();
                }))
      .function("ReadUInt32",
                optional_override([](const danet::BitStream &self) -> val {
                  uint32_t v;
                  if (self.Read<uint32_t>(v))
                    return val(v);
                  return val::null();
                }))
      .function("ReadInt64",
                optional_override([](const danet::BitStream &self) -> val {
                  int64_t v;
                  if (self.Read<int64_t>(v))
                    return val(v);
                  return val::null();
                }))
      .function("ReadUInt64",
                optional_override([](const danet::BitStream &self) -> val {
                  uint64_t v;
                  if (self.Read<uint64_t>(v))
                    return val(v);
                  return val::null();
                }))
      .function("ReadFloat",
                optional_override([](const danet::BitStream &self) -> val {
                  float v;
                  if (self.Read<float>(v))
                    return val(v);
                  return val::null();
                }))
      .function("ReadDouble",
                optional_override([](const danet::BitStream &self) -> val {
                  double v;
                  if (self.Read<double>(v))
                    return val(v);
                  return val::null();
                }))
      .function("ReadBool",
                optional_override([](const danet::BitStream &self) -> val {
                  bool v;
                  if (self.Read(v))
                    return val(v);
                  return val::null();
                }))
      .function("ReadString",
                optional_override([](const danet::BitStream &self) -> val {
                  std::string v;
                  if (self.Read(v))
                    return val(v);
                  return val::null();
                }))

      .function("WriteCompressedInt32", select_overload<void(int32_t)>(
                                            &danet::BitStream::WriteCompressed))
      .function(
          "WriteCompressedUInt32",
          select_overload<void(uint32_t)>(&danet::BitStream::WriteCompressed))
      .function("WriteCompressedInt16", select_overload<void(int16_t)>(
                                            &danet::BitStream::WriteCompressed))
      .function(
          "WriteCompressedUInt16",
          select_overload<void(uint16_t)>(&danet::BitStream::WriteCompressed))

      .function("ReadCompressedInt32",
                optional_override([](const danet::BitStream &self) -> val {
                  int32_t v;
                  if (self.ReadCompressed(v))
                    return val(v);
                  return val::null();
                }))
      .function("ReadCompressedUInt32",
                optional_override([](const danet::BitStream &self) -> val {
                  uint32_t v;
                  if (self.ReadCompressed(v))
                    return val(v);
                  return val::null();
                }))
      .function("ReadCompressedInt16",
                optional_override([](const danet::BitStream &self) -> val {
                  int16_t v;
                  if (self.ReadCompressed(v))
                    return val(v);
                  return val::null();
                }))
      .function("ReadCompressedUInt16",
                optional_override([](const danet::BitStream &self) -> val {
                  uint16_t v;
                  if (self.ReadCompressed(v))
                    return val(v);
                  return val::null();
                }));
}
