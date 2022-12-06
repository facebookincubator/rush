// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.

#pragma once

#include "Constants.h"
#include "Serializer.h"

namespace rush {

struct BaseFrame : public Serializable {
  uint64_t frameLength{0};
  const uint64_t sequenceId{0};
  const uint8_t frameType{0};

  explicit BaseFrame(rush::FrameTypes frameType, uint64_t sequenceId);
  virtual void serialize(Cursor& cursor) const override;
  virtual size_t length() const override;
};

struct ConnectFrame : public BaseFrame {
  const uint8_t version{0};
  const uint16_t audioTimescale{0};
  const uint16_t videoTimescale{0};
  const uint64_t broadcastId{0};
  const ByteStream payload;

  explicit ConnectFrame(
      uint64_t sequenceId,
      uint8_t version,
      uint16_t audioTimescale,
      uint16_t videoTimescale,
      uint64_t broadcastId,
      const ByteStream& payload);
  virtual void serialize(Cursor& cursor) const override;
  virtual size_t length() const override;
};

struct VideoWithTrackFrame : public BaseFrame {
  const uint8_t codec;
  const uint64_t pts;
  const uint64_t dts;
  const uint8_t trackId;
  const uint16_t requiredFrameOffset;
  const ByteStream data;
  const ByteStream extradata;

  explicit VideoWithTrackFrame(
      uint64_t sequenceId,
      rush::VideoCodec codec,
      uint64_t pts,
      uint64_t dts,
      uint8_t trackId,
      uint16_t requiredFrameOffset,
      const ByteStream& data,
      const ByteStream& extradata);
  virtual void serialize(Cursor& cursor) const override;
  virtual size_t length() const override;
};

struct AudioWithTrackFrame : public BaseFrame {
  const uint8_t codec;
  const uint64_t pts;
  const uint8_t trackId;
  const ByteStream data;
  const ByteStream extradata;

  explicit AudioWithTrackFrame(
      uint64_t sequenceId,
      rush::AudioCodec codec,
      uint64_t pts,
      uint8_t trackId,
      const ByteStream& data,
      const ByteStream& extradata);
  virtual void serialize(Cursor& cursor) const override;
  virtual size_t length() const override;
};

struct AudioWithHeaderFrame : public BaseFrame {
  const uint8_t codec;
  const uint64_t pts;
  const uint8_t trackId;
  const uint16_t headerLength;
  const ByteStream data;
  const ByteStream extradata;

  explicit AudioWithHeaderFrame(
      uint64_t sequenceId,
      rush::AudioCodec codec,
      uint64_t pts,
      uint8_t trackId,
      uint16_t headerLength,
      const ByteStream& data,
      const ByteStream& extradata);
  virtual void serialize(Cursor& cursor) const override;
  virtual size_t length() const override;
};

struct EndOfStreamFrame : public BaseFrame {
  explicit EndOfStreamFrame(uint64_t sequenceId);
  virtual void serialize(Cursor& cursor) const override;
  virtual size_t length() const override;
};

} // namespace rush
