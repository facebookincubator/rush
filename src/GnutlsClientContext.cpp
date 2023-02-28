// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.

#include "GnutlsClientContext.h"

#include <ngtcp2/ngtcp2.h>
#include <ngtcp2/ngtcp2_crypto_gnutls.h>

#include "QuicConnection.h"

namespace {

static const char priority[] =
    "NORMAL:-VERS-ALL:+VERS-TLS1.3:-CIPHER-ALL:+AES-128-GCM:+AES-256-GCM:"
    "+CHACHA20-POLY1305:+AES-128-CCM:-GROUP-ALL:+GROUP-SECP256R1:+GROUP-X25519:"
    "+GROUP-SECP384R1:"
    "+GROUP-SECP521R1:%DISABLE_TLS13_COMPAT_MODE";

} // namespace

constexpr unsigned char kRushAlpnV1[] = {'r', 'u', 's', 'h', '/', '3'};

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

static int hookFunction(
    gnutls_session_t session,
    unsigned int htype,
    unsigned when,
    unsigned int incoming,
    const gnutls_datum_t* msg) {
  // no-op
  return 0;
}

int GNUTLSClientContext::setConnectionId(
    ngtcp2_cid* cid,
    uint8_t* token,
    size_t cidlen) {
  if (gnutls_rnd(GNUTLS_RND_RANDOM, cid->data, cidlen)) {
    return NGTCP2_ERR_CALLBACK_FAILURE;
  }
  cid->datalen = cidlen;
  if (gnutls_rnd(GNUTLS_RND_RANDOM, token, NGTCP2_STATELESS_RESET_TOKENLEN)) {
    return NGTCP2_ERR_CALLBACK_FAILURE;
  }
  return 0;
}

int GNUTLSClientContext::generateSecureRandom(uint8_t* data, size_t datalen) {
  if (int error = gnutls_rnd(GNUTLS_RND_RANDOM, data, datalen)) {
    std::cerr << "gnutls_rnd failed" << std::endl;
    return -1;
  }
  return 0;
}

void* GNUTLSClientContext::getNativeHandle() const {
  return static_cast<void*>(session_);
}

int GNUTLSClientContext::init(
    const char* remoteHost,
    ngtcp2_crypto_conn_ref& ref) {
  if (int error = gnutls_certificate_allocate_credentials(&cred_)) {
    std::cerr << "Cred init failed " << error << gnutls_strerror(error)
              << std::endl;
    return -1;
  }

  if (int error = gnutls_certificate_set_x509_system_trust(cred_)) {
    if (error < 0) {
      // num certificates < 0 less than signals an error
      std::cerr << "Error setting gnutls certificate" << gnutls_strerror(error)
                << " " << error << std::endl;
      return -1;
    }
  }

  if (int error = gnutls_init(
          &session_,
          GNUTLS_CLIENT | GNUTLS_ENABLE_EARLY_DATA |
              GNUTLS_NO_END_OF_EARLY_DATA)) {
    std::cerr << "GNU TLS init failed " << gnutls_strerror(error);
    return -1;
  }

  if (int error = ngtcp2_crypto_gnutls_configure_client_session(session_)) {
    std::cerr << "ngtcp2_crypto_gnutls_configure_client_session failed"
              << std::endl;
    return -1;
  }

  if (int error = gnutls_priority_set_direct(session_, priority, nullptr)) {
    std::cerr << "Error setting GNU TLS priority " << gnutls_strerror(error)
              << std::endl;
    return -1;
  }

  gnutls_handshake_set_hook_function(
      session_, GNUTLS_HANDSHAKE_ANY, GNUTLS_HOOK_POST, hookFunction);

  gnutls_session_set_ptr(session_, &ref);

  if (int error =
          gnutls_credentials_set(session_, GNUTLS_CRD_CERTIFICATE, cred_)) {
    std::cerr << "Error setting GNU TLS credentials " << gnutls_strerror(error)
              << std::endl;
    return -1;
  }

  gnutls_datum_t alpn{
      .data = const_cast<unsigned char*>(kRushAlpnV1), .size = kAlpnLength};

  if (int error = gnutls_alpn_set_protocols(
          session_, &alpn, 1, GNUTLS_ALPN_MANDATORY)) {
    std::cerr << "Unable to set ALPN " << gnutls_strerror(error);
    return -1;
  }

  if (!numeric_host(remoteHost)) {
    gnutls_server_name_set(
        session_, GNUTLS_NAME_DNS, remoteHost, strlen(remoteHost));
  } else {
    gnutls_server_name_set(
        session_, GNUTLS_NAME_DNS, "localhost", strlen("localhost"));
  }

  return 0;
}

GNUTLSClientContext::~GNUTLSClientContext() {
  gnutls_deinit(session_);
  gnutls_certificate_free_credentials(cred_);
}

} // namespace rush
