// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.

#pragma once

#include <stdint.h>
#include <stdlib.h>

namespace rush {

enum class Nal : uint8_t {
  Unspecified = 0,
  Slice = 1,
  IdrSlice = 5,
  Sps = 7,
  Pps = 8,
};

// video codecs
enum class VideoCodec : uint8_t {
  H264 = 0x1,
  H265 = 0x2,
  VP8 = 0x3,
  VP9 = 0x4,
};

// audio codecs
enum class AudioCodec : uint8_t {
  Aac = 0x1,
  Opus = 0x2,
};

// frame types
enum class FrameTypes : uint8_t {
  Connect = 0x0,
  ConnectAck = 0x01,
  VideoWithTrack = 0xD,
  AudioWithTrack = 0xE,
  AudioWithHeader = 0x14,
  EndofStream = 0x4,
  Error = 0x5,
};

} // namespace rush
