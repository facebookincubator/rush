// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.

#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>

#include "Utils.h"

namespace rush {

enum class ConnectionState : uint8_t {
  Unset,
  TransportConnected,
  BroadcastAccepted,
  Failed,
  Stopped,
};

struct ConnectionSharedState {
  std::atomic<ConnectionState> state{ConnectionState::Unset};
  std::mutex stateMutex;
  std::condition_variable stateCv;
};

} // namespace rush
