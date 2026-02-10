#pragma once
#include <HalStorage.h>

#include <iostream>

namespace serialization {
template <typename T>
static void writePod(std::ostream& os, const T& value) {
  os.write(reinterpret_cast<const char*>(&value), sizeof(T));
}

template <typename T>
static void writePod(FsFile& file, const T& value) {
  file.write(reinterpret_cast<const uint8_t*>(&value), sizeof(T));
}

template <typename T>
static void readPod(std::istream& is, T& value) {
  is.read(reinterpret_cast<char*>(&value), sizeof(T));
}

template <typename T>
static void readPod(FsFile& file, T& value) {
  file.read(reinterpret_cast<uint8_t*>(&value), sizeof(T));
}

static void writeString(std::ostream& os, const std::string& s) {
  const uint32_t len = s.size();
  writePod(os, len);
  os.write(s.data(), len);
}

static void writeString(FsFile& file, const std::string& s) {
  const uint32_t len = s.size();
  writePod(file, len);
  file.write(reinterpret_cast<const uint8_t*>(s.data()), len);
}

// Max string length to prevent heap corruption from corrupted/incompatible files.
// ESP32-C3 has ~380KB heap; 64KB is a safe upper bound for any serialized string.
constexpr uint32_t MAX_SERIALIZED_STRING_LENGTH = 65536;

static bool readString(std::istream& is, std::string& s) {
  uint32_t len;
  readPod(is, len);
  if (len > MAX_SERIALIZED_STRING_LENGTH) {
    s.clear();
    return false;
  }
  if (len == 0) {
    s.clear();
    return true;
  }
  s.resize(len);
  is.read(&s[0], len);
  if (is.gcount() != static_cast<std::streamsize>(len)) {
    s.clear();
    return false;
  }
  return true;
}

static bool readString(FsFile& file, std::string& s) {
  uint32_t len;
  readPod(file, len);
  if (len > MAX_SERIALIZED_STRING_LENGTH) {
    s.clear();
    return false;
  }
  if (len == 0) {
    s.clear();
    return true;
  }
  s.resize(len);
  const int bytesRead = file.read(reinterpret_cast<uint8_t*>(&s[0]), len);
  if (bytesRead < 0 || static_cast<uint32_t>(bytesRead) != len) {
    s.clear();
    return false;
  }
  return true;
}
}  // namespace serialization
