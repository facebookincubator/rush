// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.

#pragma once

#include <arpa/inet.h>
#include <netdb.h>
#include <ngtcp2/ngtcp2.h>
#include <chrono>
#include <cstring>
#include <iostream>

namespace rush {

enum class NetworkError {
  ok = 0,
  sendBlocked,
  fatalError,
};

union sockAddrUnion {
  sockaddr_storage storage;
  sockaddr sa;
  sockaddr_in6 in6;
  sockaddr_in in;
};

struct Address {
  socklen_t len;
  union sockAddrUnion su;
};

int createSocket(
    const char* remoteHost,
    const char* remotePort,
    Address& remoteAddress);
int connectSocket(int fd, Address remoteAddress, Address& localAddress);

int createNonBlockingSocket(int domain, int type, int protocol);

int createUdpSocket(int family);

ngtcp2_tstamp timestamp();

void log_printf(void* user_data, const char* fmt, ...);

std::string logtimestamp();

std::string getIPAddress(const Address& address);

} // namespace rush
