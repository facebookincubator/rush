// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.

#pragma once

#include "Constants.h"
#include "Rush.h"
#include "Utils.h"

#include <unordered_map>

class RushMuxer {
 public:
  int addAudioStream(int timescale, uint8_t index);

  int addVideoStream(int timescale, uint8_t index);

  uint64_t getVideoTimescale() const;

  uint64_t getAudioTimescale() const;

  ssize_t
  connectFrame(uint8_t* payload, int size, uint8_t* buffer, int bufferLength);

  ssize_t videoWithTrackFrame(
      rush::VideoCodec codec,
      uint8_t index,
      bool isKeyFrame,
      uint8_t* data,
      int length,
      uint64_t pts,
      uint64_t dts,
      uint8_t* extradata,
      int extradataLength,
      uint8_t* buffer,
      int bufferLength);

  ssize_t audioWithTrackFrame(
      rush::AudioCodec codec,
      uint8_t index,
      uint8_t* data,
      int length,
      uint64_t dts,
      uint8_t* extradata,
      int extradataLength,
      uint8_t* buffer,
      int bufferLength);

  ssize_t audioWithHeaderFrame(
      rush::AudioCodec codec,
      uint8_t index,
      uint8_t* data,
      int length,
      uint64_t pts,
      uint8_t* extradata,
      int extradataLength,
      uint8_t* buffer,
      int bufferLength);

  ssize_t endOfStreamFrame(uint8_t* buffer, int bufferLength);

 private:
  uint64_t getSequenceId();

  uint16_t
  getRequiredFrameOffset(bool isKeyFrame, uint64_t sequenceId, uint8_t index);

  uint16_t audioTimescale_{0};
  uint16_t videoTimescale_{0};
  uint64_t sequenceId_{0};
  uint8_t numAudioStreams_{0};
  uint8_t numVideoStreams_{0};
  std::unordered_map<uint64_t, uint8_t> indexToTrackId_;
  std::unordered_map<uint8_t, uint64_t> indexToLastKeysequenceId_;
};
