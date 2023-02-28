// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.

#pragma once

#include <ngtcp2/ngtcp2_crypto.h>
#include <memory>

namespace rush {

class TLSClientContext {
 public:
  virtual int init(const char* address, ngtcp2_crypto_conn_ref& ref) = 0;
  virtual void* getNativeHandle() const = 0;
  virtual int generateSecureRandom(uint8_t* data, size_t datalen) = 0;
  virtual int
  setConnectionId(ngtcp2_cid* cid, uint8_t* token, size_t length) = 0;
  virtual ~TLSClientContext() = default;
};

std::unique_ptr<TLSClientContext> createTLSContext();

} // namespace rush
