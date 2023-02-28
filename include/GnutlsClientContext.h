// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.

#pragma once

#include <gnutls/crypto.h>
#include <gnutls/gnutls.h>
#include <ngtcp2/ngtcp2_crypto.h>
#include <fstream>

#include "TLSClientContext.h"

namespace rush {
class QuicConnection;

class GNUTLSClientContext : public TLSClientContext {
 public:
  ~GNUTLSClientContext();
  int init(const char* remoteHost, ngtcp2_crypto_conn_ref& ref) override;
  void* getNativeHandle() const override;
  int generateSecureRandom(uint8_t* data, size_t datalen) override;
  int setConnectionId(ngtcp2_cid* cid, uint8_t* token, size_t length) override;

 private:
  gnutls_certificate_credentials_t cred_;
  gnutls_session_t session_;
};

} // namespace rush
