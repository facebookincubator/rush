// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.

#include "OpensslClientContext.h"
#include "QuicConnection.h"
#include "TLSClientContext.h"

#include <ngtcp2/ngtcp2.h>
#include <ngtcp2/ngtcp2_crypto.h>
#include <ngtcp2/ngtcp2_crypto_openssl.h>

constexpr unsigned char kRushAlpnV1[] = {6, 'r', 'u', 's', 'h', '/', '3'};
constexpr unsigned int kAlpnLength = sizeof(kRushAlpnV1);

namespace rush {

static int numeric_host_family(const char* hostname, int family) {
  uint8_t dst[sizeof(struct in6_addr)];
  return inet_pton(family, hostname, dst) == 1;
}

static int numeric_host(const char* hostname) {
  return numeric_host_family(hostname, AF_INET) ||
      numeric_host_family(hostname, AF_INET6);
}

void* OpensslClientContext::getNativeHandle() const {
  return static_cast<void*>(ssl_);
}

OpensslClientContext::~OpensslClientContext() {
  SSL_free(ssl_);
  SSL_CTX_free(sslCtx_);
}

int OpensslClientContext::init(
    const char* remoteHost,
    ngtcp2_crypto_conn_ref& ref) {
  sslCtx_ = SSL_CTX_new(TLS_client_method());
  if (!sslCtx_) {
    std::cerr << "SSL_CTX_new failed "
              << ERR_error_string(ERR_get_error(), nullptr) << std::endl;
    return -1;
  }

  if (int error = ngtcp2_crypto_openssl_configure_client_context(sslCtx_)) {
    std::cerr << "SSL configure failes with " << error;
    return -1;
  }

  ssl_ = SSL_new(sslCtx_);
  if (!ssl_) {
    std::cerr << "SSL_new failed" << ERR_error_string(ERR_get_error(), nullptr)
              << std::endl;
    return -1;
  }

  SSL_set_app_data(ssl_, &ref);
  SSL_set_connect_state(ssl_);
  SSL_set_alpn_protos(
      ssl_, static_cast<const unsigned char*>(kRushAlpnV1), kAlpnLength);

  if (!numeric_host(remoteHost)) {
    SSL_set_tlsext_host_name(ssl_, remoteHost);
  }
  SSL_set_quic_transport_version(ssl_, TLSEXT_TYPE_quic_transport_parameters);
  return 0;
}

int OpensslClientContext::generateSecureRandom(uint8_t* data, size_t datalen) {
  if (RAND_bytes(data, static_cast<int>(datalen)) != 1) {
    return -1;
  }
  return 0;
}

int OpensslClientContext::setConnectionId(
    ngtcp2_cid* cid,
    uint8_t* token,
    size_t cidlen) {
  if (RAND_bytes(cid->data, static_cast<int>(cidlen)) != 1) {
    return -1;
  }
  cid->datalen = cidlen;
  if (RAND_bytes(token, NGTCP2_STATELESS_RESET_TOKENLEN) != 1) {
    return NGTCP2_ERR_CALLBACK_FAILURE;
  }
  return 0;
}

} // namespace rush
