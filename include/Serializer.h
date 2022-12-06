// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.

#pragma once

#include <cassert>
#include <cstring>
#include <iostream>

#include <iomanip>
#include <iostream>

namespace rush {

struct ByteStream {
  const uint8_t* data{nullptr};
  const size_t length{0};

  ByteStream() {}
  ByteStream(uint8_t* ptr, int len) : data(ptr), length(len) {}
};

template <typename T>
size_t lengthParams(T val) {
  return sizeof(val);
}

template <typename T, typename... Rest>
size_t lengthParams(T val, Rest... rest) {
  return sizeof(val) + lengthParams(rest...);
}

template <typename = ByteStream>
size_t lengthParams(ByteStream stream);

template <typename = ByteStream, typename... Rest>
size_t lengthParams(ByteStream stream, Rest... rest) {
  return stream.length + lengthParams(rest...);
}

class Cursor {
 public:
  Cursor(uint8_t* ptr, size_t size);
  bool canAdvance(size_t length);
  void advance(size_t length);

  // big endian read/writes
  template <typename T>
  void readBE(T& val);

  template <typename T>
  void writeBE(T val);

  template <typename T, typename... Rest>
  void writeBE(T val, Rest... rest) {
    writeBE(val);
    writeBE(rest...);
  }

  // little endian read/writes
  template <typename T>
  void read(T& val);

  template <typename T>
  void write(T val);

  template <typename T, typename... Rest>
  void write(T val, Rest... rest) {
    write(val);
    write(rest...);
  }

  size_t position() const;

  bool available() const;

 private:
  uint8_t* const start_{nullptr};
  uint8_t* const end_{nullptr};
  uint8_t* pos_{nullptr};
};

struct Serializable {
  virtual void serialize(Cursor& cursor) const = 0;
  virtual size_t length() const = 0;
  virtual ~Serializable() = default;
};

template <typename T>
Cursor& operator<<(Cursor& cursor, const T& frame) {
  frame.serialize(cursor);
  return cursor;
}

} // namespace rush
