// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.

#pragma once

#include <algorithm>
#include <mutex>
#include <vector>

constexpr size_t kInitalPoolSize = 1000;
constexpr size_t kNodeCapacity = 1024;

class Pool {
 public:
  struct Node {
    Node(size_t capacity);
    ~Node();
    size_t getCapacity() const;

    Node(const Node& node);
    Node& operator=(const Node& other) = delete;

    uint8_t* data{nullptr};
    size_t length{0};

   private:
    size_t capacity_{0};

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
    mutable bool copied_{false};
  };

  Pool();
  ~Pool();
  Node get();
  void free(Node&& node);

 private:
  size_t nodeSize_{kNodeCapacity};

  void grow();
  void reclaim();
  std::vector<Node> pool_;
  std::vector<Node> freed_;
  std::mutex reclaimMutex_;
};
