// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.

#include "Utils.h"

#include <cassert>
#include <ctime>
#include <iomanip>
#include <iostream>

using namespace std;

namespace rush {

int createNonBlockingSocket(int domain, int type, int protocol) {
  const int fd = socket(domain, type | SOCK_NONBLOCK, protocol);
  if (fd == -1) {
    return -1;
  }
  return fd;
}

int createUdpSocket(int family) {
  const int fd = createNonBlockingSocket(family, SOCK_DGRAM, IPPROTO_UDP);
  if (fd == -1) {
    return -1;
  }
  return fd;
}

int createSocket(
    const char* remoteHost,
    const char* remotePort,
    Address& remoteAddress) {
  addrinfo hints{};
  addrinfo *res{nullptr}, *rp{nullptr};

  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_DGRAM;

  if (const int error = getaddrinfo(remoteHost, remotePort, &hints, &res)) {
    std::cerr << "getaddrinfo fails " << gai_strerror(error) << std::endl;
    return -1;
  }

  int fd = -1;
  for (rp = res; rp; rp = rp->ai_next) {
    fd = createUdpSocket(rp->ai_family);
    if (fd == -1) {
      continue;
    }
    break;
  }

  if (fd == -1) {
    std::cerr << "Could not create socket " << std::endl;
    return -1;
  }

  remoteAddress.len = rp->ai_addrlen;
  memcpy(&remoteAddress.su, rp->ai_addr, rp->ai_addrlen);

  freeaddrinfo(res);

  return fd;
}

int connectSocket(int fd, Address remoteAddress, Address& localAddress) {
  if (connect(fd, &remoteAddress.su.sa, remoteAddress.len)) {
    std::cerr << "connect failed with [" << strerror(errno) << "]" << std::endl;
    return -1;
  }

  socklen_t len = sizeof(localAddress.su.storage);
  if (getsockname(fd, &localAddress.su.sa, &len)) {
    std::cerr << "getsockname fails [" << strerror(errno) << "]" << std::endl;

    return -1;
  }
  localAddress.len = len;
  return 0;
}

ngtcp2_tstamp timestamp() {
  auto ts = std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::steady_clock::now().time_since_epoch())
                .count();
  return ts;
}

void log_printf(void* user_data, const char* fmt, ...) {
  va_list ap;
  (void)user_data;

  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);

  fprintf(stderr, "\n");
}

std::string logtimestamp() {
  std::ostringstream strStream;
  std::time_t t = std::time(nullptr);
  strStream << "[" << std::put_time(std::localtime(&t), "%F %T %Z") << "] ";
  return strStream.str();
}

std::string getIPAddress(const Address& address) {
  if (address.su.storage.ss_family == AF_INET6) {
    char ip[INET6_ADDRSTRLEN];
    inet_ntop(AF_INET6, &(address.su.in6.sin6_addr), ip, INET6_ADDRSTRLEN);
    return std::string(ip);
  } else {
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(address.su.in.sin_addr), ip, INET_ADDRSTRLEN);
    return std::string(ip);
  }
}

} // namespace rush
