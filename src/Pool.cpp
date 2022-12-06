// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.

#include "Pool.h"

Pool::Node::Node(size_t max) : capacity_(max) {
  data = static_cast<uint8_t*>(std::malloc(capacity_));
  if (!data) {
    throw std::bad_alloc();
  }
}

Pool::Node::~Node() {
  if (data && !copied_) {
    std::free(data);
  }
}

size_t Pool::Node::getCapacity() const {
  return capacity_;
}

Pool::Node::Node(const Pool::Node& node) {
  capacity_ = node.getCapacity();
  data = node.data;
  length = node.length;

  // Node holds a pointer to an allocated memory and
  // we do not want this struct to be copyable since
  // that might lead to memory being accidentally freed
  // during destruction of the copy.
  //
  // But, this struct gets passed to the event-loop thread
  // as a part of std::function() and std::function()
  // requires Node to be CopyConstructible.
  //
  // 'copied' makes sure that we do not free the memory when
  // destroying copies of the struct.
  node.copied_ = true;
}

Pool::Pool() {
  grow();
}

Pool::~Pool() {
  pool_.clear();
}

void Pool::grow() {
  std::generate_n(std::back_inserter(pool_), kInitalPoolSize, [&]() {
    return Node(nodeSize_);
  });
}

void Pool::reclaim() {
  std::lock_guard<std::mutex> lock(reclaimMutex_);
  pool_.swap(freed_);
}

void Pool::free(Node&& node) {
  std::lock_guard<std::mutex> lock(reclaimMutex_);
  freed_.emplace_back(std::move(node));
}

Pool::Node Pool::get() {
  if (!pool_.size()) {
    reclaim();
  }
  if (!pool_.size()) {
    grow();
  }
  auto node = std::move(pool_.back());
  pool_.pop_back();
  return node;
}
