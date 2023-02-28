// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.

#include "TLSClientContext.h"

#ifdef TLS_USE_GNUTLS
#include "GnutlsClientContext.h"
#else
#include "OpensslClientContext.h"
#endif

namespace rush {

std::unique_ptr<TLSClientContext> createTLSContext() {
#ifdef TLS_USE_GNUTLS
  return std::make_unique<GNUTLSClientContext>();
#else
  return std::make_unique<OpensslClientContext>();
#endif
}

} // namespace rush
