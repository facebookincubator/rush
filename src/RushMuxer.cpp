// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.

#include "RushMuxer.h"

#include "CodecUtils.h"
#include "Frames.h"

#include <cstring>
#include <iostream>
#include <limits>

static constexpr uint16_t kMaxRequiredOffsetValue = 0xFFFF;
static constexpr uint16_t kDefaultVideoTimescale = 60000;
static constexpr uint8_t kRushVersion = 3;

using namespace rush;

bool audioCodecValid(AudioCodec codec) {
  if (codec == AudioCodec::Opus || codec == AudioCodec::Aac) {
    return true;
  }
  return false;
}

bool videoCodecValid(VideoCodec codec) {
  if (codec == VideoCodec::H264 || codec == VideoCodec::H265 ||
      codec == VideoCodec::VP8 || codec == VideoCodec::VP9) {
    return true;
  }
  return false;
}

uint64_t RushMuxer::getAudioTimescale() const {
  return audioTimescale_;
}

int RushMuxer::addAudioStream(int timescale, uint8_t index) {
  indexToTrackId_[index] = numAudioStreams_++;
  if (!audioTimescale_) {
    audioTimescale_ = static_cast<uint16_t>(timescale);
    return 0;
  }
  if (timescale != audioTimescale_) {
    std::cerr << "Two audio streams with different timescale" << std::endl;
    return -1;
  }
  return 0;
}

uint64_t RushMuxer::getVideoTimescale() const {
  return videoTimescale_;
}

int RushMuxer::addVideoStream(int timescale, uint8_t index) {
  indexToTrackId_[index] = numVideoStreams_++;
  if (!videoTimescale_) {
    const int maxSupportedTimescale =
        static_cast<int>(std::numeric_limits<uint16_t>::max());
    videoTimescale_ = timescale >= maxSupportedTimescale
        ? kDefaultVideoTimescale
        : static_cast<uint16_t>(timescale);
    return 0;
  }
  if (timescale != videoTimescale_) {
    std::cerr << "Two video streams with different timescales" << std::endl;
    return -1;
  }
  return 0;
}

uint64_t RushMuxer::getSequenceId() {
  return ++sequenceId_;
}

ssize_t RushMuxer::connectFrame(
    uint8_t* payload,
    int length,
    uint8_t* buffer,
    int bufferLength) {
  const uint64_t sequenceId = getSequenceId();
  auto const connectPayload = (ByteStream){.data = payload, .length = length};
  ConnectFrame frame(
      sequenceId,
      kRushVersion,
      videoTimescale_,
      audioTimescale_,
      1,
      connectPayload);

  Cursor writeCursor(buffer, bufferLength);
  writeCursor << frame;

  return writeCursor.position();
}

ssize_t RushMuxer::videoWithTrackFrame(
    VideoCodec codec,
    uint8_t index,
    bool isKeyFrame,
    uint8_t* data,
    int length,
    uint64_t pts,
    uint64_t dts,
    uint8_t* extradata,
    int extradataLength,
    uint8_t* buffer,
    int bufferLength) {
  if (!videoCodecValid(codec)) {
    throw std::runtime_error("Invalid video codec");
  }

  const uint64_t sequenceId = getSequenceId();
  const bool addExtradata = isKeyFrame;
  const auto sample = (ByteStream){.data = data, .length = length};
  const auto codecData = addExtradata
      ? (ByteStream){.data = extradata, .length = extradataLength}
      : ByteStream();
  const uint8_t trackId = indexToTrackId_[index];
  const uint16_t requiredFrameOffset =
      getRequiredFrameOffset(isKeyFrame, sequenceId, index);
  VideoWithTrackFrame frame(
      sequenceId,
      codec,
      pts,
      dts,
      trackId,
      requiredFrameOffset,
      sample,
      codecData);

  Cursor writeCursor(buffer, bufferLength);
  writeCursor << frame;

  return writeCursor.position();
}

uint16_t RushMuxer::getRequiredFrameOffset(
    bool isKeyFrame,
    uint64_t sequenceId,
    uint8_t index) {
  if (isKeyFrame) {
    indexToLastKeysequenceId_[index] = sequenceId;
    return 0;
  }

  const auto it = indexToLastKeysequenceId_.find(index);
  if (it == indexToLastKeysequenceId_.end()) {
    std::cerr << "WARN : No preceding key-frame for video stream" << std::endl;
    return kMaxRequiredOffsetValue;
  }

  // max allowed distance of a from key-frame for this implementation
  // is UINT16_MAX
  const uint16_t offset = static_cast<uint16_t>(sequenceId - it->second);
  if (offset >= kMaxRequiredOffsetValue) {
    throw std::runtime_error(
        "Required frame offset larger than maximum allowed");
  }

  return offset;
}

ssize_t RushMuxer::audioWithTrackFrame(
    AudioCodec codec,
    uint8_t index,
    uint8_t* data,
    int length,
    uint64_t pts,
    uint8_t* extradata,
    int extradataLength,
    uint8_t* buffer,
    int bufferLength) {
  if (!audioCodecValid(codec)) {
    throw std::runtime_error("Invalid audio codec");
  }
  const uint64_t sequenceId = getSequenceId();
  const uint8_t trackId = indexToTrackId_[index];
  const bool addExtradata = (codec == AudioCodec::Opus);
  const auto sample = (ByteStream){.data = data, .length = length};
  const auto codecData = addExtradata
      ? (ByteStream){.data = extradata, .length = extradataLength}
      : ByteStream();
  AudioWithTrackFrame frame(sequenceId, codec, pts, trackId, sample, codecData);

  Cursor writeCursor(buffer, bufferLength);
  writeCursor << frame;

  return writeCursor.position();
}

ssize_t RushMuxer::audioWithHeaderFrame(
    AudioCodec codec,
    uint8_t index,
    uint8_t* data,
    int length,
    uint64_t pts,
    uint8_t* extradata,
    int extradataLength,
    uint8_t* buffer,
    int bufferLength) {
  if (!audioCodecValid(codec)) {
    throw std::runtime_error("Invalid audio codec");
  }
  const uint64_t sequenceId = getSequenceId();
  const uint8_t trackId = indexToTrackId_[index];
  const auto sample = (ByteStream){.data = data, .length = length};
  const auto codecData =
      (ByteStream){.data = extradata, .length = extradataLength};
  const uint16_t headerLength = static_cast<uint16_t>(extradataLength);
  AudioWithHeaderFrame frame(
      sequenceId, codec, pts, trackId, headerLength, sample, codecData);

  Cursor writeCursor(buffer, bufferLength);
  writeCursor << frame;

  return writeCursor.position();
}

ssize_t RushMuxer::endOfStreamFrame(uint8_t* buffer, int bufferLength) {
  const uint64_t sequenceId = getSequenceId();
  EndOfStreamFrame frame(sequenceId);

  Cursor writeCursor(buffer, bufferLength);
  writeCursor << frame;

  return writeCursor.position();
}
