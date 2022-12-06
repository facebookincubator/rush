// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.

#pragma once

#include "Pool.h"

namespace rush {
typedef struct {
  int64_t streamID{-1};
  bool blocked{false};
  const std::unique_ptr<rush::Buffer> txBuffer{
      std::make_unique<rush::Buffer>()};
} Stream;

} // namespace rush
