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

int isAvccFit(uint8_t* data, int length);

int isAnnexb(uint8_t* data, int length);

int isHEVCConfigRecord(uint8_t* data, size_t length);

size_t getRushFrameSize(size_t sample, size_t codecData);

ssize_t getParameterFromConfigRecordHEVC(
    uint8_t* data,
    size_t length,
    uint8_t* buffer,
    size_t bufLength);

int isH264ConfigRecord(uint8_t* data, size_t length);

ssize_t getPrameterFromConfigRecordH264(
    uint8_t* data,
    size_t length,
    uint8_t* buffer,
    size_t bufLength);

int shouldProcessExtradata(
    uint8_t* data,
    int length,
    int* isKeyFrame,
    int* processExtradata);

#ifdef __cplusplus
}
#endif
