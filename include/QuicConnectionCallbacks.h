// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.

#pragma once

#include <ev.h>
#include <ngtcp2/ngtcp2.h>
#include <memory>

#include "Stream.h"

namespace rush {

typedef struct {
  size_t (*onSocketWriteable)(
      int64_t& stream,
      int& finish,
      ngtcp2_vec* vec,
      size_t vecCount,
      void* context);

  int (*onStreamDataFramed)(int64_t streamId, size_t length, void* context);

  int (*onAckedStreamDataOffset)(
      int64_t streamId,
      uint64_t offset,
      uint64_t length,
      void* context);

  int (*onRecvStreamData)(
      int64_t streamId,
      int fin,
      const uint8_t* data,
      size_t length,
      size_t& processed,
      void* context);

  int (*onStreamBlocked)(int64_t streamId, void* context);

  int (*onExtendStreamMaxData)(int64_t streamId, void* context);

  int (*bindStream)(std::unique_ptr<Stream>&& stream, void* context);

  void* context;
} QuicConnectionCallbacks;

} // namespace rush
