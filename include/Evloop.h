// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.

#pragma once

#include <ev.h>
#include <functional>
#include <mutex>
#include <queue>

namespace rush {

class EvLoop {
 public:
  EvLoop();
  ~EvLoop();
  void run(int flags);
  void enqueue(std::function<void()> f);
  int enqueueAndWait(std::function<int()> f);
  struct ev_loop* get();

 private:
  static void
  asyncWatcherCallback(struct ev_loop* loop, ev_async* w, int revents);
  static void process(std::queue<std::function<void()>>& funcs);

  struct ev_loop* loop_;

  ev_async asyncWatcher_;
  std::queue<std::function<void()>> queue_;
  std::mutex mutex_;
};

} // namespace rush
