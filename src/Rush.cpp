// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.

#include <Rush.h>
#include <cassert>
#include <iostream>

#include "RushClient.h"
#include "RushMuxer.h"

using namespace rush;

RushClientHandle createClient() {
  return new RushClient();
}

int connectTo(RushClientHandle handle, const char* host, int port) {
  return handle->connect(host, port);
}

int sendMessage(RushClientHandle handle, const uint8_t* data, int size) {
  return handle->sendMessage(data, size);
}

void destroyClient(RushClientHandle handle) {
  if (!handle) {
    return;
  }
  delete handle;
}

void rushClose(RushClientHandle handle) {
  handle->close();
}

RushMuxerHandle createMuxer() {
  return new RushMuxer();
}

void destroyMuxer(RushMuxerHandle handle) {
  if (!handle) {
    return;
  }
  delete handle;
}

int addAudioStream(RushMuxerHandle handle, int timescale, uint8_t index) {
  assert(handle);
  return handle->addAudioStream(timescale, index);
}

int addVideoStream(RushMuxerHandle handle, int timescale, uint8_t index) {
  assert(handle);
  return handle->addVideoStream(timescale, index);
}

uint64_t getAudioTimescale(RushMuxerHandle handle) {
  return handle->getAudioTimescale();
}

uint64_t getVideoTimescale(RushMuxerHandle handle) {
  return handle->getVideoTimescale();
}

ssize_t connectFrame(
    RushMuxerHandle handle,
    uint8_t* payload,
    int size,
    uint8_t* buffer,
    int bufLength) {
  assert(handle);
  return handle->connectFrame(payload, size, buffer, bufLength);
}

ssize_t videoWithTrackFrame(
    RushMuxerHandle handle,
    uint8_t codec,
    uint8_t index,
    int isKeyFrame,
    uint8_t* data,
    int length,
    uint64_t pts,
    uint64_t dts,
    uint8_t* extradata,
    int extradataLength,
    uint8_t* buffer,
    int bufLength) {
  assert(handle);
  return handle->videoWithTrackFrame(
      static_cast<VideoCodec>(codec),
      index,
      isKeyFrame <= 0 ? false : true,
      data,
      length,
      pts,
      dts,
      extradata,
      extradataLength,
      buffer,
      bufLength);
}

ssize_t audioWithTrackFrame(
    RushMuxerHandle handle,
    uint8_t codec,
    uint8_t index,
    uint8_t* data,
    int length,
    uint64_t dts,
    uint8_t* extradata,
    int extradataLength,
    uint8_t* buffer,
    int bufLength) {
  assert(handle);
  return handle->audioWithTrackFrame(
      static_cast<AudioCodec>(codec),
      index,
      data,
      length,
      dts,
      extradata,
      extradataLength,
      buffer,
      bufLength);
}

ssize_t audioWithHeaderFrame(
    RushMuxerHandle handle,
    uint8_t codec,
    uint8_t index,
    uint8_t* data,
    int length,
    uint64_t dts,
    uint8_t* extradata,
    int extradataLength,
    uint8_t* buffer,
    int bufLength) {
  assert(handle);
  return handle->audioWithHeaderFrame(
      static_cast<AudioCodec>(codec),
      index,
      data,
      length,
      dts,
      extradata,
      extradataLength,
      buffer,
      bufLength);
}

ssize_t
endOfStreamFrame(RushMuxerHandle handle, uint8_t* buffer, int bufLength) {
  return handle->endOfStreamFrame(buffer, bufLength);
}
