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

static std::ofstream keylogFile{};

namespace rush {
class QuicConnection;

class TLSClientContext {
 public:
  ~TLSClientContext();
  int init(const char* remoteHost, ngtcp2_crypto_conn_ref& ref);
  SSL* getNativeHandle() const;
  void enableKeylog(const char* filename);

 private:
  SSL_CTX* sslCtx_{nullptr};
  SSL* ssl_{nullptr};
};
} // namespace rush
