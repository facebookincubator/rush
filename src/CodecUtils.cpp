// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.

#include "CodecUtils.h"

#include "Constants.h"
#include "Serializer.h"
#include "Utils.h"

using namespace rush;

static constexpr uint32_t kHEVCConfigRecordMinLength = 22;
static constexpr size_t kMaxRushHeaderSize = 40;

size_t getRushFrameSize(size_t sample, size_t codecData) {
  return sample + codecData + kMaxRushHeaderSize;
}

int isAnnexb(uint8_t* data, int length) {
  Cursor readCursor(data, length);
  try {
    uint32_t fourByteCode{0};
    readCursor.readBE(fourByteCode);
    const uint32_t threeByteCode = fourByteCode >> 8;
    if (fourByteCode == 1 || threeByteCode == 1) {
      return 1;
    }
    return 0;
  } catch (const std::out_of_range& error) {
    return 1;
  }
  return 1;
}

int isAvccFit(uint8_t* data, int length) {
  Cursor readCursor(data, length);
  try {
    do {
      uint32_t naluLength{0};
      readCursor.readBE(naluLength);
      if (!naluLength) {
        return 0;
      }
      if (!readCursor.canAdvance(naluLength)) {
        return 0;
      }
      readCursor.advance(naluLength);
    } while (readCursor.available());
  } catch (const std::out_of_range& ex) {
    return 0;
  }
  return 1;
}

int isHEVCConfigRecord(uint8_t* data, size_t length) {
  if (!data) {
    return 1;
  }
  if (length < kHEVCConfigRecordMinLength) {
    return 0;
  }

  Cursor readCursor(data, length);
  uint32_t distanceToReservedBit{13};

  try {
    if (!readCursor.canAdvance(distanceToReservedBit)) {
      return 0;
    }
    uint8_t reserved{0};
    readCursor.advance(distanceToReservedBit);
    readCursor.read(reserved);
    if (reserved & 0xF0) {
      return 1;
    }
    return 0;
  } catch (const std::out_of_range& error) {
    return 1;
  }
}

ssize_t getParameterFromConfigRecordHEVC(
    uint8_t* data,
    size_t length,
    uint8_t* buffer,
    size_t bufferLength) {
  Cursor writeCursor(buffer, bufferLength);
  Cursor readCursor(data, length);

  // bits
  // 8    configuration version (always 0x1)
  // 2    general profile space
  // 1    general tier flag
  // 5    general profile idc
  // 32   general profile compatibility flag
  // 48   general constraint indicator flag
  // 8    general level idc
  // 4    reserved ( all bits on )
  // 2    parallelism type
  // 6    reserved ( all bits on )
  // 2    chroma format idc
  // 5    reserved ( all bits on )
  // 3    bit depth luma minus 8
  // 5    reserved ( all bits on )
  // 3    bit depth chroma minus 8
  // 16   average frame rate
  // 2    constant frame rate
  // 3    num temporal layers
  // 2    length size minus one
  // 8    num of arrays
  //
  // repeated once per num of arrays
  //      1 array completeness
  //      1 reserved ( always 0x0 )
  //      6 NAL unit type
  //      16 num NALUs
  //      repeated per NALU
  //          16 NAL unit size
  //          variable NALU data

  if (!readCursor.canAdvance(21)) {
    std::cerr << "Malformed HEVC config record" << std::endl;
    return -1;
  }

  readCursor.advance(21); // skip unnecessary bytes

  try {
    uint8_t naluPrefixLengthMinusOne{0};
    readCursor.read(naluPrefixLengthMinusOne);
    const uint8_t naluPrefixLength =
        static_cast<uint8_t>((naluPrefixLengthMinusOne & 0x03) + 1);
    if (naluPrefixLength != 4) {
      std::cerr << "Only 4 byte prefixed NALU are supported" << std::endl;
      return -1;
    }
    uint8_t numArrays{0};
    readCursor.read(numArrays);
    if (!numArrays) {
      std::cerr << "Could not parse num arrays" << std::endl;
      return -1;
    }
    for (uint8_t i = 0; i < numArrays; ++i) {
      if (!readCursor.canAdvance(1)) {
        std::cerr << "Malformed HEVC config record" << std::endl;
        return -1;
      }
      readCursor.advance(1);
      uint16_t numNalus{0};
      readCursor.readBE(numNalus);
      if (!numNalus) {
        std::cerr << "Could not parse num NALUs" << std::endl;
        return -1;
      }
      for (uint16_t j = 0; j < numNalus; ++j) {
        uint16_t naluLength{0};
        readCursor.readBE(naluLength);
        if (!readCursor.canAdvance(naluLength)) {
          std::cerr << "NALU length " << naluLength << " not valid"
                    << std::endl;
          return -1;
        }
        // write NALU as a 4 byte length prefixed NALU in network byte-order
        writeCursor.writeBE(static_cast<uint32_t>(naluLength));
        writeCursor.write(ByteStream(data + readCursor.position(), naluLength));
        readCursor.advance(naluLength);
      }
    }
  } catch (const std::out_of_range& error) {
    std::cerr << "Error processing HEVC Config Record" << std::endl;
    return -1;
  }

  return writeCursor.position();
}

int isH264ConfigRecord(uint8_t* data, size_t length) {
  if (!data) {
    return 0;
  }
  return data[0] == 1; // Avcc Configuration record marker
}

ssize_t getPrameterFromConfigRecordH264(
    uint8_t* data,
    size_t length,
    uint8_t* buffer,
    size_t bufferLength) {
  Cursor writeCursor(buffer, bufferLength);
  Cursor readCursor(data, length);

  // bits
  //  8   version ( always 0x01 )
  //  8   avc profile
  //  8   avc compatibility
  //  8   avc level
  //  6   reserved ( all bits on )
  //  2   NALU length minus 1
  //  3   reserved ( all bits on )
  //  5   number of SPS NALUs (usually 1)
  //
  //  repeated once per SPS:
  //    16         SPS size
  //    variable   SPS NALU data
  //
  //  8   number of PPS NALUs (usually 1)
  //
  //  repeated once per PPS:
  //    16       PPS size
  //    variable PPS NALU data

  if (!readCursor.canAdvance(4)) {
    std::cerr << "Malformed AVCC config record" << std::endl;
    return -1;
  }

  readCursor.advance(4); // skip unnecessary bytes

  try {
    uint8_t naluPrefixLengthMinusOne{0};
    readCursor.read(naluPrefixLengthMinusOne);
    const uint8_t naluPrefixLength =
        static_cast<uint8_t>((naluPrefixLengthMinusOne & 0x03) + 1);
    if (naluPrefixLength != 4) {
      std::cerr << "Only 4 bytes prefixed NALU are supported" << std::endl;
      return -1;
    }
    for (int i = 0; i < 2; ++i) {
      uint8_t numNalus{0};
      readCursor.read(numNalus);
      numNalus = numNalus & 0x1F;
      for (int j = 0; j < numNalus; ++j) {
        uint16_t naluLength{0};
        readCursor.readBE(naluLength);
        if (!readCursor.canAdvance(naluLength)) {
          std::cerr << "NALU length " << naluLength << " not valid"
                    << std::endl;
          return -1;
        }
        // write NALU as a 4 byte length prefixed NALU in network byte-order
        writeCursor.writeBE(static_cast<uint32_t>(naluLength));
        writeCursor.write(ByteStream(data + readCursor.position(), naluLength));
        readCursor.advance(naluLength);
      }
    }
  } catch (const std::out_of_range& error) {
    std::cerr << "Could not parse H264 Config Record" << std::endl;
    return -1;
  }
  return writeCursor.position();
}

int shouldProcessExtradata(
    uint8_t* data,
    int length,
    int* isKeyFrame,
    int* processExtradata) {
  Cursor readCursor(data, length);

  uint32_t naluLength{0};
  bool sps{false}, pps{false}, idr{false};

  while (readCursor.available()) {
    if (!readCursor.canAdvance(sizeof(naluLength))) {
      std::cerr << "Invalid length NALU" << std::endl;
      return -1;
    }
    try {
      naluLength = 0;
      readCursor.readBE(naluLength);
    } catch (const std::out_of_range& error) {
      std::cerr << "Invalid NALU length" << std::endl;
      return -1;
    }
    if (!readCursor.canAdvance(naluLength)) {
      std::cerr << "Invalid NALU length" << std::endl;
      return -1;
    }

    const uint8_t peek = data[readCursor.position()];
    const auto nalType = static_cast<Nal>((peek)&0x1F);

    switch (nalType) {
      case Nal::Sps:
        sps = true;
        break;
      case Nal::Pps:
        pps = true;
        break;
      case Nal::IdrSlice:
        idr = true;
        break;
      default:
        break;
    }
    readCursor.advance(naluLength);

    if (sps && pps && idr) {
      break;
    }
  }

  *isKeyFrame = idr;

  // sps and pps are contained in-band, don't process
  // extradata
  if (sps || pps) {
    *processExtradata = false;
    return 0;
  }

  // key frame present or processing the first frame
  if (*isKeyFrame) {
    *processExtradata = true;
    return 0;
  }

  return 0;
}
