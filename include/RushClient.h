// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.

#pragma once

#include <thread>

#include "Buffer.h"
#include "ConnectionState.h"
#include "Evloop.h"
#include "Pool.h"
#include "QuicConnection.h"
#include "QuicConnectionCallbacks.h"
#include "Utils.h"

class RushClient {
 public:
  int connect(const char* hostname, int port);
  int sendMessage(const uint8_t* data, size_t length);
  int close();

  size_t onSocketWriteable(
      int64_t& stream,
      int& finish,
      ngtcp2_vec* vec,
      size_t vecCount);

  int onStreamDataFramed(int64_t streamId, size_t dataLength);

  int onAckedStreamDataOffset(
      int64_t streamId,
      uint64_t offset,
      uint64_t dataLength);

  int onRecvStreamData(
      int64_t streamId,
      int fin,
      const uint8_t* data,
      size_t dataLength,
      size_t& processed);

  int bindStream(std::unique_ptr<rush::Stream>&& stream);

  int onStreamBlocked(int64_t streamId);

  int onExtendStreamMaxData(int64_t streamId);

 private:
  void writeToBuffer(Pool::Node&& node);
  void changeState(rush::ConnectionState state);

  Pool pool_;
  std::thread thread_;
  std::shared_ptr<rush::QuicConnection> conn_;
  const std::shared_ptr<rush::EvLoop> loop_{std::make_shared<rush::EvLoop>()};
  const std::shared_ptr<rush::ConnectionSharedState> connstate_{
      std::make_shared<rush::ConnectionSharedState>()};
  std::unique_ptr<rush::Stream> stream_;
  bool connectSent_{false};
};
