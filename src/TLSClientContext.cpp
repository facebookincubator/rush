// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.

#include "TLSClientContext.h"

#include <ngtcp2/ngtcp2.h>
#include <ngtcp2/ngtcp2_crypto.h>
#include <ngtcp2/ngtcp2_crypto_openssl.h>

#include "QuicConnection.h"

#define ALPN "\x2h3"

namespace {

static void keylog(const SSL* ssl, const char* line) {
  keylogFile.write(line, strlen(line));
  keylogFile.put('\n');
  keylogFile.flush();
}

} // namespace

namespace rush {

static int numeric_host_family(const char* hostname, int family) {
  uint8_t dst[sizeof(struct in6_addr)];
  return inet_pton(family, hostname, dst) == 1;
}

static int numeric_host(const char* hostname) {
  return numeric_host_family(hostname, AF_INET) ||
      numeric_host_family(hostname, AF_INET6);
}

void TLSClientContext::enableKeylog(const char* logfile) {
  keylogFile.open(logfile, std::ios_base::app);
  SSL_CTX_set_keylog_callback(sslCtx_, keylog);
}

SSL* TLSClientContext::getNativeHandle() const {
  return ssl_;
}

TLSClientContext::~TLSClientContext() {
  SSL_free(ssl_);
  SSL_CTX_free(sslCtx_);
}

int TLSClientContext::init(
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
  SSL_set_alpn_protos(ssl_, (const unsigned char*)ALPN, sizeof(ALPN) - 1);

  if (!numeric_host(remoteHost)) {
    SSL_set_tlsext_host_name(ssl_, remoteHost);
  }
  SSL_set_quic_transport_version(ssl_, TLSEXT_TYPE_quic_transport_parameters);
  return 0;
}

} // namespace rush
