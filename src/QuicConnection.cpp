// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.

#include "QuicConnection.h"
#include "RushClient.h"

#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/ssl.h>
#include <unistd.h>
#include <algorithm>
#include <cassert>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <random>

static constexpr float kPrintStatsSeconds = 60.;
static constexpr uint32_t kSendBatchSize = 10;

namespace {

static int extendMaxLocalBidirectionalStreamsCb(
    ngtcp2_conn* conn,
    uint64_t maxStreams,
    void* userData) {
  auto* client = static_cast<rush::QuicConnection*>(userData);
  if (client->onExtendMaxStreams()) {
    return NGTCP2_ERR_CALLBACK_FAILURE;
  }
  return 0;
}

static void
randomCb(uint8_t* dest, size_t destlen, const ngtcp2_rand_ctx* ctx) {
  (void)ctx;
  for (size_t i = 0; i < destlen; ++i) {
    *dest = (uint8_t)random();
  }
}

static int getNewConnectionIdCb(
    ngtcp2_conn* conn,
    ngtcp2_cid* cid,
    uint8_t* token,
    size_t cidlen,
    void* userData) {
  (void)conn;
  (void)userData;

  if (RAND_bytes(cid->data, (int)cidlen) != 1) {
    return NGTCP2_ERR_CALLBACK_FAILURE;
  }

  cid->datalen = cidlen;

  if (RAND_bytes(token, NGTCP2_STATELESS_RESET_TOKENLEN) != 1) {
    return NGTCP2_ERR_CALLBACK_FAILURE;
  }
  return 0;
}

static int generateSecureRandom(uint8_t* data, size_t datalen) {
  if (RAND_bytes(data, static_cast<int>(datalen)) != 1) {
    return -1;
  }
  return 0;
}

static int onHandshakeComplete(ngtcp2_conn* conn, void* userData) {
  auto* client = static_cast<rush::QuicConnection*>(userData);
  if (client->onHandshakeComplete()) {
    return NGTCP2_ERR_CALLBACK_FAILURE;
  }
  return 0;
}

static int extendMaxStreamDataCb(
    ngtcp2_conn* conn,
    int64_t streamID,
    uint64_t maxData,
    void* userData,
    void* streamUserData) {
  auto* client = static_cast<rush::QuicConnection*>(userData);
  if (client->onExtendStreamMaxData(streamID)) {
    return NGTCP2_ERR_CALLBACK_FAILURE;
  }
  return 0;
}

static int recvStreamDataCb(
    ngtcp2_conn* conn,
    uint32_t flags,
    int64_t streamID,
    uint64_t offset,
    const uint8_t* data,
    size_t datalen,
    void* userData,
    void* streamUserData) {
  auto* client = static_cast<rush::QuicConnection*>(userData);
  if (client->recvStreamData(flags, streamID, data, datalen)) {
    return NGTCP2_ERR_CALLBACK_FAILURE;
  }
  return 0;
}

static int ackStreamDataCb(
    ngtcp2_conn* conn,
    int64_t streamID,
    uint64_t offset,
    uint64_t datalen,
    void* userData,
    void* streamUserData) {
  auto* client = static_cast<rush::QuicConnection*>(userData);
  if (client->ackStreamData(
          conn, streamID, offset, datalen, userData, streamUserData)) {
    return NGTCP2_ERR_CALLBACK_FAILURE;
  }
  return 0;
}

static ngtcp2_conn* getConnectionCb(ngtcp2_crypto_conn_ref* ref) {
  auto* client = static_cast<rush::QuicConnection*>(ref->user_data);
  return client->getConnection();
}

} // namespace

namespace {
static void readCallback(struct ev_loop* loop, ev_io* w, int revents) {
  auto client = static_cast<rush::QuicConnection*>(w->data);
  if (client->onRead()) {
    return;
  }
  client->onWrite();
}

static void writeCallback(struct ev_loop* loop, ev_io* w, int revents) {
  auto client = static_cast<rush::QuicConnection*>(w->data);
  if (client->onWrite()) {
    return;
  }
}

static void timeoutCallback(struct ev_loop* loop, ev_timer* w, int revents) {
  auto client = static_cast<rush::QuicConnection*>(w->data);
  if (client->handleExpiry()) {
    return;
  }
  client->onWrite();
}

static void statsCallback(struct ev_loop* loop, ev_timer* w, int revents) {
  auto client = static_cast<rush::QuicConnection*>(w->data);
  client->printStats();
}

} // namespace

namespace rush {

QuicConnection::QuicConnection(
    struct ev_loop* loop,
    int fd,
    Address localAddress,
    Address remoteAddress,
    QuicConnectionCallbacks callbacks,
    std::shared_ptr<ConnectionSharedState> sharedConnectionState)
    : loop_(loop),
      fd_(fd),
      localAddress_(localAddress),
      remoteAddress_(remoteAddress),
      callbacks_(callbacks),
      connstate_(sharedConnectionState) {}

int QuicConnection::connect() {
  connRef_.get_conn = ::getConnectionCb;
  connRef_.user_data = this;

  if (tls_.init(getIPAddress(remoteAddress_).c_str(), connRef_)) {
    std::cerr << "TLS init error" << std::endl;
    return -1;
  }

  ngtcp2_callbacks callbacks = {
      ngtcp2_crypto_client_initial_cb,
      nullptr, /* recv_client_initial */
      ngtcp2_crypto_recv_crypto_data_cb,
      ::onHandshakeComplete, /* handshake_completed */
      nullptr, /* recv_version_negotiation */
      ngtcp2_crypto_encrypt_cb,
      ngtcp2_crypto_decrypt_cb,
      ngtcp2_crypto_hp_mask_cb,
      ::recvStreamDataCb, /* recv_stream_data */
      ::ackStreamDataCb, /* acked_stream_data_offset */
      nullptr, /* stream_open */
      nullptr, /* stream_close */
      nullptr, /* recv_stateless_reset */
      ngtcp2_crypto_recv_retry_cb,
      ::extendMaxLocalBidirectionalStreamsCb,
      nullptr, /* extend_max_local_streams_uni */
      randomCb,
      getNewConnectionIdCb,
      nullptr, /* remove_connection_id */
      ngtcp2_crypto_update_key_cb,
      nullptr, /* path_validation */
      nullptr, /* select_preferred_address */
      nullptr, /* stream_reset */
      nullptr, /* extend_max_remote_streams_bidi */
      nullptr, /* extend_max_remote_streams_uni */
      ::extendMaxStreamDataCb, /* extend_max_stream_data */
      nullptr, /* dcid_status */
      nullptr, /* handshake_confirmed */
      nullptr, /* recv_new_token */
      ngtcp2_crypto_delete_crypto_aead_ctx_cb,
      ngtcp2_crypto_delete_crypto_cipher_ctx_cb,
      nullptr, /* recv_datagram */
      nullptr, /* ack_datagram */
      nullptr, /* lost_datagram */
      ngtcp2_crypto_get_path_challenge_data_cb,
      nullptr, /* stream_stop_sending */
      ngtcp2_crypto_version_negotiation_cb,
      nullptr, /* receive rx key*/
      nullptr, /* receive tx key*/
      nullptr, /* early data rejected*/
  };

  // settings
  ngtcp2_settings settings;
  ngtcp2_settings_default(&settings);
  settings.initial_ts = timestamp();
  settings.cc_algo = NGTCP2_CC_ALGO_BBR;

  // transport params
  ngtcp2_transport_params params;
  ngtcp2_transport_params_default(&params);
  params.initial_max_streams_uni = 3;
  params.initial_max_streams_bidi = 3;
  params.initial_max_stream_data_bidi_local = 1024 * 1024;
  params.initial_max_stream_data_bidi_remote = 1024 * 1024;
  params.initial_max_data = 1024 * 1024;
  params.max_idle_timeout = 300 * NGTCP2_SECONDS;

  ngtcp2_cid scid, dcid;
  scid.datalen = 8;
  if (generateSecureRandom(scid.data, scid.datalen)) {
    std::cerr << "Could not generate source connection id" << std::endl;
    return -1;
  }

  dcid.datalen = NGTCP2_MIN_INITIAL_DCIDLEN;
  if (generateSecureRandom(dcid.data, dcid.datalen)) {
    std::cerr << "Could not generate destination connection id" << std::endl;
    return -1;
  }

  auto path = ngtcp2_path{
      {
          const_cast<sockaddr*>(&localAddress_.su.sa),
          localAddress_.len,
      },
      {
          const_cast<sockaddr*>(&remoteAddress_.su.sa),
          remoteAddress_.len,
      },
      nullptr};

  if (int error = ngtcp2_conn_client_new(
          &conn_,
          &dcid,
          &scid,
          &path,
          NGTCP2_PROTO_VER_V1,
          &callbacks,
          &settings,
          &params,
          nullptr,
          this)) {
    std::cerr << "could not create ngtcp2 client " << ngtcp2_strerror(error)
              << std::endl;

    return -1;
  }

  ngtcp2_conn_set_tls_native_handle(conn_, tls_.getNativeHandle());

  ev_io_init(&readEv_, ::readCallback, fd_, EV_READ);
  readEv_.data = this;
  ev_io_start(loop_, &readEv_);

  ev_io_init(&writeEv_, ::writeCallback, fd_, EV_WRITE);
  writeEv_.data = this;
  ev_io_start(loop_, &writeEv_);

  ev_timer_init(&timer_, ::timeoutCallback, 0., 0.);
  timer_.data = this;

  ev_timer_init(&statsTimer_, ::statsCallback, kPrintStatsSeconds, 0.);
  statsTimer_.data = this;
  statsTimer_.repeat = kPrintStatsSeconds;
  ev_timer_again(loop_, &statsTimer_);

  return 0;
}

QuicConnection::~QuicConnection() {
  disconnect();
  if (conn_) {
    ngtcp2_conn_del(conn_);
  }
}

void QuicConnection::changeState(ConnectionState state) {
  {
    std::lock_guard<std::mutex> guard(connstate_->stateMutex);
    connstate_->state.store(state, std::memory_order_relaxed);
  }
  connstate_->stateCv.notify_all();
}

int QuicConnection::onExtendMaxStreams() {
  auto stream = std::make_unique<rush::Stream>();
  if (int error =
          ngtcp2_conn_open_bidi_stream(conn_, &stream->streamID, nullptr)) {
    return error;
  }
  if (callbacks_.bindStream) {
    callbacks_.bindStream(std::move(stream), callbacks_.context);
  }
  return 0;
}

int QuicConnection::onHandshakeComplete() {
  changeState(ConnectionState::TransportConnected);
  return 0;
}

int QuicConnection::onExtendStreamMaxData(int64_t streamId) {
  if (callbacks_.onExtendStreamMaxData) {
    return callbacks_.onExtendStreamMaxData(streamId, callbacks_.context);
  }
  return 0;
}

ngtcp2_conn* QuicConnection::getConnection() {
  return conn_;
}

int QuicConnection::recvStreamData(
    uint32_t flags,
    int64_t streamId,
    const uint8_t* data,
    size_t datalen) {
  size_t processed{datalen};
  if (callbacks_.onRecvStreamData) {
    if (int error = callbacks_.onRecvStreamData(
            streamId,
            flags & NGTCP2_STREAM_DATA_FLAG_FIN,
            data,
            datalen,
            processed,
            callbacks_.context)) {
      if (error) {
        std::cerr << "Error processing received data. Disconnecting"
                  << std::endl;
        disconnect();
      }
      return error;
    }
  }
  if (processed) {
    ngtcp2_conn_extend_max_stream_offset(conn_, streamId, datalen);
    ngtcp2_conn_extend_max_offset(conn_, datalen);
  }
  return 0;
}

int QuicConnection::ackStreamData(
    ngtcp2_conn* conn,
    int64_t streamId,
    uint64_t offset,
    uint64_t datalen,
    void* userData,
    void* streamUserData) {
  if (callbacks_.onAckedStreamDataOffset) {
    return callbacks_.onAckedStreamDataOffset(
        streamId, offset, datalen, callbacks_.context);
  }
  return 0;
}

size_t QuicConnection::getBuffer(
    int64_t& streamId,
    int& finish,
    ngtcp2_vec* datavec,
    size_t datavecCount) {
  if (callbacks_.onSocketWriteable) {
    const size_t vecs = callbacks_.onSocketWriteable(
        streamId, finish, datavec, datavecCount, callbacks_.context);
    if (vecs > 0) {
      return vecs;
    }
  }
  streamId = -1;
  finish = 0;
  return 0;
}

ngtcp2_connection_close_error* QuicConnection::getLastError() {
  return &lastError_;
}

int QuicConnection::onWrite() {
  const ngtcp2_tstamp ts = timestamp();
  const size_t payloadSize = ngtcp2_conn_get_max_tx_udp_payload_size(conn_);
  ngtcp2_pkt_info packetInfo;
  ngtcp2_path_storage pathStorage;
  int64_t streamId{-1};
  int finish{0};
  std::array<uint8_t, 65535> buffer;
  size_t pkts{0};

  ngtcp2_path_storage_zero(&pathStorage);

  for (;;) {
    std::array<ngtcp2_vec, 16> datavec;
    size_t datavecCount{0};
    datavecCount = getBuffer(streamId, finish, datavec.data(), datavec.size());

    uint32_t flags = NGTCP2_WRITE_STREAM_FLAG_MORE;
    if (finish) {
      flags |= NGTCP2_WRITE_STREAM_FLAG_FIN;
    }

    ngtcp2_ssize appWrite{0};

    ngtcp2_ssize totalWrite = ngtcp2_conn_writev_stream(
        conn_,
        &pathStorage.path,
        &packetInfo,
        buffer.data(),
        payloadSize,
        &appWrite,
        flags,
        streamId,
        datavec.data(),
        datavecCount,
        ts);

    if (totalWrite < 0) {
      switch (totalWrite) {
        case NGTCP2_ERR_WRITE_MORE:
          if (callbacks_.onStreamDataFramed) {
            callbacks_.onStreamDataFramed(
                streamId, appWrite, callbacks_.context);
          }
          continue;
        case NGTCP2_ERR_STREAM_DATA_BLOCKED:
          if (callbacks_.onStreamBlocked) {
            callbacks_.onStreamBlocked(streamId, callbacks_.context);
          }
          continue;
        default:
          std::cerr << logtimestamp() << "ngtcp2_conn_writev_stream "
                    << ngtcp2_strerror(static_cast<int>(totalWrite))
                    << std::endl;
          ngtcp2_connection_close_error_set_transport_error_liberr(
              &lastError_, static_cast<int>(totalWrite), nullptr, 0);
          disconnect();
      }
    }

    if (totalWrite == 0) {
      ngtcp2_conn_update_pkt_tx_time(conn_, ts);
      return 0;
    }

    if (appWrite > 0) {
      if (callbacks_.onStreamDataFramed) {
        callbacks_.onStreamDataFramed(streamId, appWrite, callbacks_.context);
      }
    }

    auto error = sendPacket(buffer.data(), static_cast<size_t>(totalWrite));
    if (error != NetworkError::ok) {
      break;
    }

    // Packet Pacing: Quic recommends against sending traffic
    // in bursts. This implementationadds a short delay between
    // outgoing data
    if (++pkts == kSendBatchSize) {
      ngtcp2_conn_update_pkt_tx_time(conn_, ts);
      ev_io_start(loop_, &writeEv_);
      break;
    }
  }
  updateTimer();
  return 0;
}

int QuicConnection::onRead() {
  std::array<uint8_t, 65536> buffer;
  struct sockaddr_storage addr;
  iovec io = {buffer.data(), buffer.size()};
  msghdr msg{};

  ngtcp2_path path;
  ngtcp2_pkt_info packetInfo;

  msg.msg_name = &addr;
  msg.msg_iov = &io;
  msg.msg_iovlen = 1;
  msg.msg_namelen = sizeof(addr);

  ssize_t nRead{0};
  for (;;) {
    nRead = recvmsg(fd_, &msg, MSG_DONTWAIT);

    if (nRead == -1) {
      if (errno != EAGAIN && errno != EWOULDBLOCK) {
        std::cerr << "recvmsg error " << strerror(errno) << std::endl;
        disconnect();
      }
      break;
    }

    path.local.addrlen = localAddress_.len;
    path.local.addr = const_cast<sockaddr*>(&localAddress_.su.sa);

    path.remote.addrlen = msg.msg_namelen;
    path.remote.addr = static_cast<sockaddr*>(msg.msg_name);

    int error = ngtcp2_conn_read_pkt(
        conn_,
        &path,
        &packetInfo,
        buffer.data(),
        static_cast<size_t>(nRead),
        timestamp());

    if (error) {
      std::cerr << logtimestamp() << "ngtcp2_conn_read_pkt "
                << ngtcp2_strerror(error) << std::endl;
      switch (error) {
        case NGTCP2_ERR_REQUIRED_TRANSPORT_PARAM:
        case NGTCP2_ERR_MALFORMED_TRANSPORT_PARAM:
        case NGTCP2_ERR_TRANSPORT_PARAM:
        case NGTCP2_ERR_PROTO:
          ngtcp2_connection_close_error_set_transport_error_liberr(
              &lastError_, error, nullptr, 0);
          disconnect();
          break;
        default:
          if (lastError_.error_code) {
            ngtcp2_connection_close_error_set_transport_error_liberr(
                &lastError_, error, nullptr, 0);
            disconnect();
          }
          break;
      }
      return -1;
    }
  }
  updateTimer();
  return 0;
}

NetworkError QuicConnection::sendPacket(
    const uint8_t* data,
    size_t datalength) {
  iovec io{const_cast<uint8_t*>(data), datalength};
  msghdr msg{};
  msg.msg_iov = &io;
  msg.msg_iovlen = 1;

  ssize_t nWrite{0};

  do {
    nWrite = sendmsg(fd_, &msg, 0);
  } while (nWrite == -1 && errno == EINTR);

  if (nWrite == -1) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      return NetworkError::sendBlocked;
    }
    if (errno == EMSGSIZE) {
      return NetworkError::ok;
    }
    return NetworkError::fatalError;
  }

  assert(static_cast<size_t>(nWrite) == datalength);
  return NetworkError::ok;
}

int QuicConnection::updateTimer() {
  const auto expiry = ngtcp2_conn_get_expiry(conn_);
  const auto now = timestamp();

  if (expiry <= now) {
    ev_feed_event(loop_, &timer_, EV_TIMER);
    return 0;
  }
  const auto future = static_cast<ev_tstamp>(expiry - now) / NGTCP2_SECONDS;
  timer_.repeat = future;
  ev_timer_again(loop_, &timer_);
  return 0;
}

int QuicConnection::handleExpiry() {
  const auto now = timestamp();
  if (int error = ngtcp2_conn_handle_expiry(conn_, now)) {
    std::cerr << "ngtcp2_conn_handle_expiry " << ngtcp2_strerror(error)
              << std::endl;
    ngtcp2_connection_close_error_set_transport_error_liberr(
        &lastError_, error, nullptr, 0);
    disconnect();
    return -1;
  }
  return 0;
}

int QuicConnection::handleError() {
  if (ngtcp2_conn_is_in_closing_period(conn_) ||
      ngtcp2_conn_is_in_draining_period(conn_)) {
    return 0;
  }
  std::array<uint8_t, NGTCP2_MAX_UDP_PAYLOAD_SIZE> buffer;

  ngtcp2_path_storage pathStorage;
  ngtcp2_path_storage_zero(&pathStorage);
  ngtcp2_pkt_info packetInfo;

  auto nWrite = ngtcp2_conn_write_connection_close(
      conn_,
      &pathStorage.path,
      &packetInfo,
      buffer.data(),
      buffer.size(),
      &lastError_,
      timestamp());

  // ngtcp2_strerror(...) expects an int but
  // ngtcp2_conn_write_connection_close(...) returns a ssize_t
  if (nWrite < 0) {
    std::cerr << logtimestamp() << ngtcp2_strerror(static_cast<int>(nWrite))
              << std::endl;
    return -1;
  }

  if (nWrite == 0) {
    return 0;
  }

  return sendPacket(buffer.data(), nWrite) == NetworkError::ok ? 0 : -1;
}

int QuicConnection::disconnect() {
  if (connstate_->state == ConnectionState::Stopped) {
    return 0;
  }
  handleError();

  ev_io_stop(loop_, &writeEv_);
  ev_io_stop(loop_, &readEv_);
  ev_timer_stop(loop_, &timer_);
  ev_timer_stop(loop_, &statsTimer_);

  changeState(ConnectionState::Stopped);
  ev_break(loop_, EVBREAK_ALL);

  close(fd_);

  return 0;
}

void QuicConnection::printStats() {
  if (!conn_) {
    return;
  }
  ngtcp2_conn_stat stat;
  ngtcp2_conn_get_conn_stat(conn_, &stat);
  std::cout << "Bits per second   " << stat.delivery_rate_sec * 8 << std::endl;
  std::cout << "Congestion Window " << stat.cwnd << std::endl;
  ev_timer_again(loop_, &statsTimer_);
}

} // namespace rush
