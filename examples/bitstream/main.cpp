#include <bitstream/bitstream.h>
#include <debug/dag_assert.h>
#include <expected>
#include <iomanip>
#include <memory/dag_mem.h>
#include <print>
#include <sstream>
#include <string>
#include <vector>

void (*dgs_fatal_report)(const char *, const char *) = nullptr;

struct player_data {
  uint32_t player_id;
  float health;
  bool is_alive;
};
static_assert(eastl::is_trivially_copyable_v<player_data>);

void print_hex(const danet::BitStream &stream) {
  std::stringstream hex;
  hex << std::hex << std::setfill('0');
  for (size_t i = 0; i < stream.GetNumberOfBytesUsed(); ++i) {
    hex << std::setw(2) << static_cast<int>(stream.GetData()[i]) << " ";
  }
  std::println("{}", hex.str());
}

enum class read_error { stream_ended_prematurely };

template <typename T>
std::expected<T, read_error> read_value(danet::BitStream &stream) {
  T value;
  if (stream.Read(value)) {
    return value;
  }
  return std::unexpected(read_error::stream_ended_prematurely);
}

template <typename T>
std::expected<T, read_error> read_compressed_value(danet::BitStream &stream) {
  T value;
  if (stream.ReadCompressed(value)) {
    return value;
  }
  return std::unexpected(read_error::stream_ended_prematurely);
}

int main() {
  dagor_force_init_memmgr();

  danet::BitStream write_stream;

  write_stream.Write(uint8_t(42));
  write_stream.Write(std::string("hello bitstream"));
  write_stream.Write(true);
  write_stream.WriteCompressed(-12345);
  player_data player_one = {101, 98.6f, true};
  write_stream.Write(player_one);

  std::println("\nSerialized data ({} bytes):", write_stream.GetNumberOfBytesUsed());
  print_hex(write_stream);

  danet::BitStream read_stream(write_stream.GetData(),
                               write_stream.GetNumberOfBytesUsed(), false);

  auto a = read_value<uint8_t>(read_stream);
  auto b = read_value<std::string>(read_stream);
  auto c = read_value<bool>(read_stream);
  auto d = read_compressed_value<int32_t>(read_stream);
  auto e = read_value<player_data>(read_stream);

  if (a && b && c && d && e) {
    std::println("\nDeserialized data:");
    std::println("  uint8_t: {}", *a);
    std::println("  string: '{}'", *b);
    std::println("  bool: {}", *c);
    std::println("  compressed int32: {}", *d);
    std::println("  player_data: id={}, health={}, alive={}", e->player_id,
                 e->health, e->is_alive);
  } else {
    std::println(stderr, "Failed to read from bitstream.");
    return 1;
  }

  return 0;
}