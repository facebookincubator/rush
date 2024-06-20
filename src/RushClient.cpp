// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.

#include "RushClient.h"

#include <algorithm>
#include <cassert>
#include <chrono>

#include "Constants.h"
#include "Evloop.h"
#include "QuicConnection.h"
#include "Serializer.h"

using namespace rush;

static constexpr uint32_t kConnectAckTimeoutSeconds = 10;

namespace {

static size_t onSocketWriteable(
    int64_t& streamId,
    int& finish,
    ngtcp2_vec* vec,
    size_t vecCount,
    void* context) {
  const auto client = static_cast<RushClient*>(context);
  return client->onSocketWriteable(streamId, finish, vec, vecCount);
}

static int onStreamDataFramed(int64_t streamId, size_t length, void* context) {
  const auto client = static_cast<RushClient*>(context);
  return client->onStreamDataFramed(streamId, length);
}

static int onAckedStreamDataOffset(
    int64_t streamId,
    uint64_t offset,
    uint64_t length,
    void* context) {
  const auto client = static_cast<RushClient*>(context);
  return client->onAckedStreamDataOffset(streamId, offset, length);
}

static int onRecvStreamData(
    int64_t streamId,
    int fin,
    const uint8_t* data,
    size_t length,
    size_t& processed,
    void* context) {
  const auto client = static_cast<RushClient*>(context);
  return client->onRecvStreamData(streamId, fin, data, processed, length);
}

static int bindStream(std::unique_ptr<Stream>&& stream, void* context) {
  const auto client = static_cast<RushClient*>(context);
  return client->bindStream(std::move(stream));
}

static int onStreamBlocked(int64_t streamId, void* context) {
  const auto client = static_cast<RushClient*>(context);
  return client->onStreamBlocked(streamId);
}

static int onExtendStreamMaxData(int64_t streamId, void* context) {
  const auto client = static_cast<RushClient*>(context);
  return client->onExtendStreamMaxData(streamId);
}

} // namespace

int RushClient::connect(const char* hostname, int port) {
  Address remoteAddress, localAddress;
  const int fd =
      createSocket(hostname, std::to_string(port).c_str(), remoteAddress);

  if (fd == -1) {
    std::cerr << "Error creating socket [" << errno << "]";
    return -1;
  }

  if (int error = connectSocket(fd, remoteAddress, localAddress)) {
    std::cerr << "Error connecting to [" << hostname << ":" << port
              << "]. Errno [" << errno << "]";
    return error;
  }

  assert(!conn_);

  QuicConnectionCallbacks callbacks{
      .onSocketWriteable = ::onSocketWriteable,
      .onStreamDataFramed = ::onStreamDataFramed,
      .onAckedStreamDataOffset = ::onAckedStreamDataOffset,
      .onRecvStreamData = ::onRecvStreamData,
      .onStreamBlocked = ::onStreamBlocked,
      .onExtendStreamMaxData = ::onExtendStreamMaxData,
      .bindStream = ::bindStream,
      .context = this,
  };

  conn_ = std::make_shared<rush::QuicConnection>(
      loop_->get(), fd, localAddress, remoteAddress, callbacks, connstate_);

  std::thread t([=]() { loop_->run(0); });
  thread_ = std::move(t);

  if (int error = loop_->enqueueAndWait([&]() { return conn_->connect(); })) {
    std::cerr << "Connect failed" << std::endl;
    return -1;
  }

  std::unique_lock<std::mutex> lock(connstate_->stateMutex);
  connstate_->stateCv.wait(
      lock, [&] { return connstate_->state != ConnectionState::Unset; });

  if (connstate_->state != ConnectionState::TransportConnected) {
    thread_.join();
    return -1;
  }
  return fd;
}

size_t RushClient::onSocketWriteable(
    int64_t& streamId,
    int& finish,
    ngtcp2_vec* vec,
    size_t vecCount) {
  if (!stream_) {
    return 0;
  }
  if (stream_->blocked) {
    return 0;
  }
  if (!stream_->txBuffer->available()) {
    return 0;
  }
  streamId = stream_->streamID;
  return stream_->txBuffer->getData(vec, vecCount);
}

int RushClient::onRecvStreamData(
    int64_t streamId,
    int fin,
    const uint8_t* data,
    size_t length,
    size_t& processed) {
  try {
    // TODO ngtcp2_recv_stream_data requires data to be const uint8_t*
    // but we can safely unconst it since we are not manipulating
    // it. Implement a read-only version of 'Cursor' in future
    Cursor cursor(const_cast<uint8_t*>(data), length);

    uint64_t frameSize{0};
    uint64_t sequeneId{0};
    uint8_t type{0};

    cursor.readBE(frameSize);
    cursor.readBE(sequeneId);
    cursor.read(type);

    const auto frameType = static_cast<FrameTypes>(type);

    switch (frameType) {
      case FrameTypes::ConnectAck: {
        std::cerr << "connect ack frame" << std::endl;
        changeState(ConnectionState::BroadcastAccepted);
        break;
      }
      case FrameTypes::Error:
        std::cerr << "Error frame" << std::endl;
        return -1;
        break;
      default:
        // cast 'type' to uint16_t to log it correctly
        std::cerr << "unrecognized frame of type "
                  << static_cast<uint16_t>(type) << std::endl;
        break;
    }
    processed = length;
  } catch (const std::out_of_range&) {
    processed = 0;
  }

  return 0;
}

int RushClient::onAckedStreamDataOffset(
    int64_t streamId,
    uint64_t offset,
    uint64_t length) {
  auto nodes = stream_->txBuffer->purge(length);
  for (auto& node : nodes) {
    pool_.free(std::move(node));
  }
  return 0;
}

int RushClient::onStreamDataFramed(int64_t streamId, size_t length) {
  stream_->txBuffer->moveCursor(length);
  return 0;
}

int RushClient::bindStream(std::unique_ptr<Stream>&& stream) {
  stream_ = std::move(stream);
  return 0;
}

int RushClient::onStreamBlocked(int64_t streamId) {
  stream_->blocked = true;
  return 0;
}

int RushClient::onExtendStreamMaxData(int64_t streamId) {
  stream_->blocked = false;
  return 0;
}

void RushClient::writeToBuffer(Pool::Node&& node) {
  stream_->txBuffer->insert(std::move(node));
}

int RushClient::sendMessage(const uint8_t* data, size_t size) {
  const auto state = connstate_->state.load(std::memory_order_relaxed);
  if (state != ConnectionState::TransportConnected &&
      state != ConnectionState::BroadcastAccepted) {
    return -1;
  }
  size_t written{0};
  while (written < size) {
    auto node = std::move(pool_.get());
    const size_t towrite = std::min(node.getCapacity(), (size - written));
    std::memcpy(node.data, data + written, towrite);
    node.length = towrite;
    loop_->enqueue([&, node = std::move(node)]() mutable {
      this->writeToBuffer(std::move(node));
    });
    written += towrite;
  }

  // TODO Add support for chunked frames. The current implementation assumes
  // sendMessage is called once per frame and does not support frame
  // fragmentation Add an implementation to keep track of frame boundaries

  // Assume the first frame is a connect frame and wait for connect-ack from
  // the broadcast server
  if (!connectSent_) {
    // connect frame sent
    connectSent_ = true;

    bool waitStatus{false};

    // Wait until we receive a connect-ack or we timeout waiting for one
    {
      std::unique_lock<std::mutex> lock(connstate_->stateMutex);
      waitStatus = connstate_->stateCv.wait_for(
          lock, std::chrono::seconds(kConnectAckTimeoutSeconds), [&] {
            return connstate_->state != ConnectionState::TransportConnected;
          });
    }

    // Timed out while waiting for connect. Preemptively closing the connection
    if (!waitStatus) {
      std::cerr << "Timed-out while waiting for connect acknowledgement"
                << std::endl;
      loop_->enqueueAndWait([&]() { return conn_->disconnect(); });
      std::cerr << "Disconnected sent" << std::endl;
      return -1;
    }

    // Received an Error frame. Connection is closed as part of the standard
    // error frame handling
    if (connstate_->state != ConnectionState::BroadcastAccepted) {
      std::cerr << "Error while waiting for connect acknowledgement"
                << std::endl;
      return -1;
    }
  }
  return 0;
}

void RushClient::changeState(ConnectionState state) {
  {
    std::lock_guard<std::mutex> guard(connstate_->stateMutex);
    connstate_->state = state;
  }
  connstate_->stateCv.notify_all();
}

int RushClient::close() {
  thread_.join();
  return 0;
}
