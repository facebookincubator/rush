// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.

#pragma once

#include <ngtcp2/ngtcp2_crypto.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/ssl.h>
#include <fstream>

#include "TLSClientContext.h"

static std::ofstream keylogFile{};

namespace rush {
class QuicConnection;

class OpensslClientContext : public TLSClientContext {
 public:
  ~OpensslClientContext();
  int init(const char* remoteHost, ngtcp2_crypto_conn_ref& ref) override;
  void* getNativeHandle() const override;
  int generateSecureRandom(uint8_t* data, size_t datalen) override;
  int setConnectionId(ngtcp2_cid* cid, uint8_t* token, size_t length) override;

 private:
  SSL_CTX* sslCtx_{nullptr};
  SSL* ssl_{nullptr};
};
} // namespace rush
