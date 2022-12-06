// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.

#include "Frames.h"

namespace rush {

BaseFrame::BaseFrame(rush::FrameTypes type, uint64_t sequenceId)
    : sequenceId(sequenceId), frameType(static_cast<uint8_t>(type)) {}

void BaseFrame::serialize(Cursor& cursor) const {
  cursor.writeBE(frameLength, sequenceId, frameType);
}

size_t BaseFrame::length() const {
  return lengthParams(frameLength, sequenceId, frameType);
}

ConnectFrame::ConnectFrame(
    uint64_t sequenceId,
    uint8_t version,
    uint16_t videoTimescale,
    uint16_t audioTimescale,
    uint64_t broadcastId,
    const ByteStream& payload)
    : BaseFrame(rush::FrameTypes::Connect, sequenceId),
      version(version),
      audioTimescale(audioTimescale),
      videoTimescale(videoTimescale),
      broadcastId(broadcastId),
      payload(payload) {
  this->frameLength = length();
}

void ConnectFrame::serialize(Cursor& cursor) const {
  BaseFrame::serialize(cursor);
  cursor.writeBE(version, videoTimescale, audioTimescale, broadcastId, payload);
}

size_t ConnectFrame::length() const {
  return BaseFrame::length() +
      lengthParams(
             version, videoTimescale, audioTimescale, broadcastId, payload);
}

VideoWithTrackFrame::VideoWithTrackFrame(
    uint64_t sequenceId,
    rush::VideoCodec codec,
    uint64_t pts,
    uint64_t dts,
    uint8_t trackId,
    uint16_t requiredFrameOffset,
    const ByteStream& data,
    const ByteStream& extradata)
    : BaseFrame(rush::FrameTypes::VideoWithTrack, sequenceId),
      codec(static_cast<uint8_t>(codec)),
      pts(pts),
      dts(dts),
      trackId(trackId),
      requiredFrameOffset(requiredFrameOffset),
      data(data),
      extradata(extradata) {
  this->frameLength = length();
}

void VideoWithTrackFrame::serialize(Cursor& cursor) const {
  BaseFrame::serialize(cursor);
  cursor.writeBE(
      codec, pts, dts, trackId, requiredFrameOffset, extradata, data);
}

size_t VideoWithTrackFrame::length() const {
  return BaseFrame::length() +
      lengthParams(
             codec, pts, dts, trackId, requiredFrameOffset, extradata, data);
}

AudioWithTrackFrame::AudioWithTrackFrame(
    uint64_t sequenceId,
    rush::AudioCodec codec,
    uint64_t pts,
    uint8_t trackId,
    const ByteStream& data,
    const ByteStream& extradata)
    : BaseFrame(rush::FrameTypes::AudioWithTrack, sequenceId),
      codec(static_cast<uint8_t>(codec)),
      pts(pts),
      trackId(trackId),
      data(data),
      extradata(extradata) {
  this->frameLength = length();
}

void AudioWithTrackFrame::serialize(Cursor& cursor) const {
  BaseFrame::serialize(cursor);
  cursor.writeBE(codec, pts, trackId, extradata, data);
}

size_t AudioWithTrackFrame::length() const {
  return BaseFrame::length() +
      lengthParams(codec, pts, trackId, extradata, data);
}

AudioWithHeaderFrame::AudioWithHeaderFrame(
    uint64_t sequenceId,
    rush::AudioCodec codec,
    uint64_t pts,
    uint8_t trackId,
    uint16_t headerLength,
    const ByteStream& data,
    const ByteStream& extradata)
    : BaseFrame(rush::FrameTypes::AudioWithHeader, sequenceId),
      codec(static_cast<uint8_t>(codec)),
      pts(pts),
      trackId(trackId),
      headerLength(headerLength),
      data(data),
      extradata(extradata) {
  this->frameLength = length();
}

void AudioWithHeaderFrame::serialize(Cursor& cursor) const {
  BaseFrame::serialize(cursor);
  cursor.writeBE(codec, pts, trackId, headerLength, extradata, data);
}

size_t AudioWithHeaderFrame::length() const {
  return BaseFrame::length() +
      lengthParams(codec, pts, trackId, headerLength, extradata, data);
}

EndOfStreamFrame::EndOfStreamFrame(uint64_t sequenceId)
    : BaseFrame(rush::FrameTypes::EndofStream, sequenceId) {
  this->frameLength = length();
}

void EndOfStreamFrame::serialize(Cursor& cursor) const {
  BaseFrame::serialize(cursor);
}

size_t EndOfStreamFrame::length() const {
  return BaseFrame::length();
}

} // namespace rush
