// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.

#include <Serializer.h>

namespace rush {

template <>
size_t lengthParams(ByteStream stream) {
  return stream.length;
}

Cursor::Cursor(uint8_t* data, size_t length)
    : start_(data), end_(data + length), pos_{start_} {}

bool Cursor::canAdvance(size_t length) {
  return pos_ + length <= end_;
}

void Cursor::advance(size_t length) {
  pos_ += length;
}

size_t Cursor::position() const {
  return pos_ - start_;
}

bool Cursor::available() const {
  if (!start_) {
    return false;
  }
  return pos_ < end_;
}

template <>
void Cursor::write<ByteStream>(ByteStream stream) {
  if (!canAdvance(stream.length)) {
    throw std::out_of_range("invalid range");
  }
  std::memcpy(pos_, stream.data, stream.length);
  advance(stream.length);
}

template <>
void Cursor::write<uint8_t>(uint8_t val) {
  if (!canAdvance(sizeof(val))) {
    throw std::out_of_range("invalid range");
  }
  pos_[0] = val;
  advance(sizeof(val));
}

template <>
void Cursor::write<uint16_t>(uint16_t val) {
  if (!canAdvance(sizeof(val))) {
    throw std::out_of_range("invalid range");
  }
  pos_[0] = static_cast<uint8_t>(val);
  pos_[1] = static_cast<uint8_t>(val >> 8);
  advance(sizeof(val));
}

template <>
void Cursor::write<uint32_t>(uint32_t val) {
  if (!canAdvance(sizeof(val))) {
    throw std::out_of_range("invalid range");
  }
  pos_[0] = static_cast<uint8_t>(val);
  pos_[1] = static_cast<uint8_t>(val >> 8);
  pos_[2] = static_cast<uint8_t>(val >> 16);
  pos_[3] = static_cast<uint8_t>(val >> 24);
  advance(sizeof(val));
}

template <>
void Cursor::write<uint64_t>(uint64_t val) {
  if (!canAdvance(sizeof(val))) {
    throw std::out_of_range("invalid range");
  }
  write<uint32_t>(static_cast<uint32_t>(val & 0xFFFFFFFF));
  write<uint32_t>(static_cast<uint32_t>(val >> 32));
}

// TODO Specializations for ByteStream and uint8_t
// are same for both big-endian and little-endian,
// and are redundant. Looking into consolidating
// these

template <>
void Cursor::writeBE<ByteStream>(ByteStream stream) {
  if (!canAdvance(stream.length)) {
    throw std::out_of_range("invalid range");
  }
  std::memcpy(pos_, stream.data, stream.length);
  advance(stream.length);
}

template <>
void Cursor::writeBE<uint8_t>(uint8_t val) {
  if (!canAdvance(sizeof(val))) {
    throw std::out_of_range("invalid range");
  }
  pos_[0] = val;
  advance(sizeof(val));
}

template <>
void Cursor::writeBE<uint16_t>(uint16_t val) {
  if (!canAdvance(sizeof(val))) {
    throw std::out_of_range("invalid range");
  }
  pos_[0] = static_cast<uint8_t>(val >> 8);
  pos_[1] = static_cast<uint8_t>(val);
  advance(sizeof(val));
}

template <>
void Cursor::writeBE<uint32_t>(uint32_t val) {
  if (!canAdvance(sizeof(val))) {
    throw std::out_of_range("invalid range");
  }
  pos_[0] = static_cast<uint8_t>(val >> 24);
  pos_[1] = static_cast<uint8_t>(val >> 16);
  pos_[2] = static_cast<uint8_t>(val >> 8);
  pos_[3] = static_cast<uint8_t>(val);
  advance(sizeof(val));
}

template <>
void Cursor::writeBE<uint64_t>(uint64_t val) {
  if (!canAdvance(sizeof(val))) {
    throw std::out_of_range("invalid range");
  }
  writeBE<uint32_t>(static_cast<uint32_t>(val >> 32));
  writeBE<uint32_t>(static_cast<uint32_t>(val & 0xFFFFFFFF));
}

template <>
void Cursor::readBE<uint32_t>(uint32_t& n) {
  if (!canAdvance(sizeof(n))) {
    throw std::out_of_range("invalid range");
  }
  const uint8_t n0 = pos_[0];
  const uint8_t n1 = pos_[1];
  const uint8_t n2 = pos_[2];
  const uint8_t n3 = pos_[3];
  n = (n0 << 24) | (n1 << 16) | (n2 << 8) | n3;
  advance(sizeof(n));
}

template <>
void Cursor::readBE<uint64_t>(uint64_t& n) {
  if (!canAdvance(sizeof(n))) {
    throw std::out_of_range("invalid range");
  }
  uint32_t n0{0}, n1{0};
  readBE(n0);
  readBE(n1);
  n = (static_cast<uint64_t>(n0) << 32) | (n1);
}

template <>
void Cursor::readBE<uint16_t>(uint16_t& n) {
  if (!canAdvance(sizeof(n))) {
    throw std::out_of_range("invalid range");
  }
  const uint8_t n0 = pos_[0];
  const uint8_t n1 = pos_[1];
  n = static_cast<uint16_t>((n0 << 8) | (n1));
  advance(sizeof(n));
}

template <>
void Cursor::read<uint8_t>(uint8_t& n) {
  if (!canAdvance(sizeof(n))) {
    throw std::out_of_range("invalid range");
  }
  n = pos_[0];
  advance(sizeof(n));
}

template <>
void Cursor::read<uint32_t>(uint32_t& n) {
  if (!canAdvance(sizeof(n))) {
    throw std::out_of_range("invalid range");
  }
  const uint8_t n0 = pos_[0];
  const uint8_t n1 = pos_[1];
  const uint8_t n2 = pos_[2];
  const uint8_t n3 = pos_[3];
  n = (n3 << 24) | (n2 << 16) | (n1 << 8) | n0;
  advance(sizeof(n));
}

template <>
void Cursor::read<uint64_t>(uint64_t& n) {
  if (!canAdvance(sizeof(n))) {
    throw std::out_of_range("invalid range");
  }
  uint32_t n0{0}, n1{0};
  read(n0);
  read(n1);
  n = (static_cast<uint64_t>(n1) << 32) | (n0);
}

} // namespace rush
