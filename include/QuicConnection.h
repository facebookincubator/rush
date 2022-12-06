// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.

#pragma once

#include <ngtcp2/ngtcp2.h>
#include <ngtcp2/ngtcp2_crypto.h>
#include <ngtcp2/ngtcp2_crypto_openssl.h>
#include <fstream>

#include "Buffer.h"
#include "ConnectionState.h"
#include "NonCopyable.h"
#include "QuicConnectionCallbacks.h"
#include "Stream.h"
#include "TLSClientContext.h"
#include "Utils.h"

namespace rush {

class QuicConnection : private NonCopyable {
 public:
  QuicConnection(
      struct ev_loop* loop,
      int fd,
      Address localAddress,
      Address remoteAddress,
      QuicConnectionCallbacks callbacks,
      std::shared_ptr<ConnectionSharedState> sharedConnectionState);

  ~QuicConnection();

  int connect();
  int disconnect();
  int onExtendMaxStreams();
  int onHandshakeComplete();
  int onExtendStreamMaxData(int64_t streamID);
  int recvStreamData(
      uint32_t flags,
      int64_t streamID,
      const uint8_t* data,
      size_t datalen);
  int ackStreamData(
      ngtcp2_conn* conn,
      int64_t stream_id,
      uint64_t offset,
      uint64_t datalen,
      void* userData,
      void* streamUserData);

  int onRead();
  int onWrite();
  int handleExpiry();
  void printStats();

  ngtcp2_conn* getConnection();
  ngtcp2_connection_close_error* getLastError();

 private:
  void changeState(ConnectionState state);
  size_t getBuffer(
      int64_t& streamID,
      int& finish,
      ngtcp2_vec* dataVector,
      size_t dataVectorSize);
  int updateTimer();
  int handleError();
  NetworkError sendPacket(const uint8_t* data, size_t dataLength);

  ngtcp2_conn* conn_{nullptr};
  ngtcp2_crypto_conn_ref connRef_{};
  ngtcp2_connection_close_error lastError_;
  struct ev_loop* loop_{nullptr};
  const int fd_{-1};
  const Address localAddress_;
  const Address remoteAddress_;
  TLSClientContext tls_;
  ev_io readEv_;
  ev_io writeEv_;
  ev_timer timer_;
  ev_timer statsTimer_;
  const QuicConnectionCallbacks callbacks_;
  const std::shared_ptr<ConnectionSharedState> connstate_;
};

} // namespace rush
