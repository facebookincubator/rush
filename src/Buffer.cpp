// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.

#include "Buffer.h"

namespace rush {

Buffer::Buffer() {
  witer_ = queue_.end();
}

Buffer::~Buffer() {
  queue_.clear();
}

int Buffer::insert(Pool::Node&& node) {
  queue_.emplace_back(std::move(node));
  if (witer_ == queue_.end()) {
    std::advance(witer_, -1);
  }
  return 0;
}

size_t Buffer::getData(ngtcp2_vec* vec, size_t vcount) {
  auto iter = witer_;
  size_t vindex = 0;
  if (offset_) {
    vec[vindex].base = const_cast<uint8_t*>((*iter).data + offset_);
    vec[vindex].len = (*iter).length - offset_;
    ++vindex;
    ++iter;
  }

  while (vindex < vcount && iter != queue_.end()) {
    vec[vindex].base = const_cast<uint8_t*>((*iter).data);
    vec[vindex].len = (*iter).length;
    ++vindex;
    ++iter;
  }
  return vindex;
}

int Buffer::moveCursor(uint64_t bytes) {
  uint64_t remaining = 0;
  while (bytes) {
    remaining = (*witer_).length - offset_;
    if (bytes < remaining) {
      offset_ += bytes;
      break;
    }
    bytes -= remaining;
    ++witer_;
    offset_ = 0;
  }
  return 0;
}

std::vector<Pool::Node> Buffer::purge(uint64_t bytes) {
  uint64_t remaining = 0;
  std::vector<Pool::Node> freeNodes;
  while (bytes) {
    auto iter = queue_.begin();
    remaining = (*iter).length - ackoffset_;
    if (bytes < remaining) {
      ackoffset_ += bytes;
      break;
    }
    bytes -= remaining;
    freeNodes.emplace_back(std::move(queue_.front()));
    queue_.pop_front();
    ackoffset_ = 0;
  }
  return freeNodes;
}

bool Buffer::available() {
  return witer_ != queue_.end();
}

} // namespace rush
