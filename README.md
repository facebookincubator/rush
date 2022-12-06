## RUSH

RUSH (Reliable - unreliable - Streaming Protocol) is a bidirectional application-level protocol designed for live video ingestion that runs on top of QUIC. RUSH was built as a replacement for RTMP (Real-Time Messaging Protocol) with the goal to provide support for new audio and video codecs, extensibility in the form of new message types, and multi-track support. In addition, RUSH gives applications the option to control data delivery guarantees by utilizing QUIC streams. See [RUSH draft](https://www.ietf.org/archive/id/draft-kpugin-rush-00.html).

### Building this Library

#### Prerequisites
This library relies on [ngtcp2](https://github.com/ngtcp2/ngtcp2) for QUIC support.


### Building on Nix
```
$ https://github.com/facebookincubator/rush.git
$ cd rush
$ export PKG_CONFIG_PATH=/openssl/build/lib/pkgconfig:/ngtcp2/build/lib/pkgconfig
$ cmake -DCMAKE_INSTALL_PREFIX=$PWD/build -DBUILD_SHARED_LIBS=OFF ..
$ make; make install
```

### Project Roadmap

The project current is under active development and the future roadmap includes:
 - Server-side RUSH implementation
 - Multi-stream mode
 - Integration with popular streaming projects
 - Support for all Operating Systems

### License

RUSH is release under the [MIT License](https://github.com/facebookincubator/rush/blob/master/LICENSE).
