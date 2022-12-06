// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.

#include "Evloop.h"

#include <condition_variable>

namespace rush {

EvLoop::EvLoop() : loop_(ev_loop_new(EVFLAG_AUTO)) {
  ev_async_init(&asyncWatcher_, EvLoop::asyncWatcherCallback);
  asyncWatcher_.data = this;
  ev_async_start(loop_, &asyncWatcher_);
}

EvLoop::~EvLoop() {
  if (loop_) {
    ev_loop_destroy(loop_);
  }
}

void EvLoop::asyncWatcherCallback(
    struct ev_loop* loop,
    ev_async* w,
    int revents) {
  EvLoop* ev = static_cast<EvLoop*>(w->data);
  std::queue<std::function<void()>> functions;
  {
    std::lock_guard<std::mutex> guard(ev->mutex_);
    functions.swap(ev->queue_);
  }
  process(functions);
}

void EvLoop::process(std::queue<std::function<void()>>& functions) {
  while (functions.size()) {
    const auto& func = functions.front();
    func();
    functions.pop();
  }
}

void EvLoop::run(int flags) {
  ev_run(loop_, flags);
}

struct ev_loop* EvLoop::get() {
  return loop_;
}

void EvLoop::enqueue(std::function<void()> f) {
  {
    std::lock_guard<std::mutex> guard(mutex_);
    queue_.push(f);
  }
  ev_async_send(loop_, &asyncWatcher_);
}

int EvLoop::enqueueAndWait(std::function<int()> f) {
  std::mutex funcMutex;
  std::condition_variable funcCv;
  int error{-1};
  bool ready{false};

  enqueue([&error, &ready, &funcMutex, &funcCv, func = std::move(f)]() {
    error = func();
    {
      std::lock_guard<std::mutex> lock(funcMutex);
      ready = true;
      funcCv.notify_all();
    }
  });

  std::unique_lock<std::mutex> lock(funcMutex);
  funcCv.wait(lock, [&ready] { return ready; });

  return error;
}

} // namespace rush
