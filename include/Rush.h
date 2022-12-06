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

#ifdef __cplusplus
extern "C" {
#endif

// RUSH Transport client
struct RushClient;

typedef struct RushClient* RushClientHandle;

RushClientHandle createClient(void);

int connectTo(RushClientHandle handle, const char* host, int port);

int sendMessage(RushClientHandle handle, const uint8_t* data, int size);

void destroyClient(RushClientHandle handle);

void rushClose(RushClientHandle handle);

// RUSH Muxer
struct RushMuxer;

typedef struct RushMuxer* RushMuxerHandle;

RushMuxerHandle createMuxer(void);

void destroyMuxer(RushMuxerHandle handle);

int addAudioStream(RushMuxerHandle handle, int timescale, uint8_t index);

int addVideoStream(RushMuxerHandle handle, int timescale, uint8_t index);

uint64_t getVideoTimescale(RushMuxerHandle handle);

uint64_t getAudioTimescale(RushMuxerHandle handle);

ssize_t connectFrame(
    RushMuxerHandle handle,
    uint8_t* payload,
    int size,
    uint8_t* buffer,
    int bufLength);

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
    int bufLength);

ssize_t audioWithTrackFrame(
    RushMuxerHandle handle,
    uint8_t codec,
    uint8_t index,
    uint8_t* data,
    int length,
    uint64_t pts,
    uint8_t* extradata,
    int extradataLength,
    uint8_t* buffer,
    int bufLength);

ssize_t audioWithHeaderFrame(
    RushMuxerHandle handle,
    uint8_t codec,
    uint8_t index,
    uint8_t* data,
    int length,
    uint64_t pts,
    uint8_t* extradata,
    int extradataLength,
    uint8_t* buffer,
    int bufLength);

ssize_t
endOfStreamFrame(RushMuxerHandle handle, uint8_t* buffer, int bufLength);

#ifdef __cplusplus
}
#endif
