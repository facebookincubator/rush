// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.

#pragma once

#include "Pool.h"

#include <ngtcp2/ngtcp2.h>
#include <list>
#include <memory>

namespace rush {

using NodeList = std::list<Pool::Node>;

class Buffer {
 public:
  Buffer();
  ~Buffer();
  int insert(Pool::Node&& node);
  size_t getData(ngtcp2_vec* vec, size_t vecCount);
  int moveCursor(
      uint64_t bytesWritten); // n bytes written to ngtcp2 buffer successfully
  std::vector<Pool::Node> purge(
      uint64_t bytesWritten); // n bytes acked from network successfully
  bool available();

 private:
  // position of write index relative to the buffer of element at witer
  int64_t offset_{0};

  // position of ack index relative to the buffer of element at begin
  int64_t ackoffset_{0};

  // write iterator, points to next element that needs to be written
  NodeList::const_iterator witer_{};

  NodeList queue_;
};

} // namespace rush
