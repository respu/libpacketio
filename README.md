# libpacketio

libpacketio is a high-performance packet I/O userspace library for networking
applications. While there are many platform specific packet I/O APIs, none of
them are suitable for writing portable applications. libpacketio aims to remedy
that by providing an API that abstracts the platform specific details while
exposing capabilities of modern NICs to applications.

## Features

### Core

 * [x] Asynchronous packet I/O API
 * [ ] SMP and multi-queue support

### Packet I/O

 * [x] Linux TPACKET (receive only)
 * [ ] DPDK
 * [ ] Netmap
 * [ ] PF_RING ZC

## Building

As a prequisite, you need to have CMake installed on your system.

First, generate makefiles with:

```sh
cmake .
```

then build ``libpacketio.a`` and auxiliary tools:

```sh
make
```

There's a ``packet-trace`` example application you can try out:

```sh
sudo ./packet-trace 
```

## License

Copyright © 2015 Pekka Enberg

libpacketio is distributed under the 2-clause BSD license.

## Refereces

Emmerich, Paul, et al. "Comparison of frameworks for high-performance packet
IO." Architectures for Networking and Communications Systems (ANCS), 2015
ACM/IEEE Symposium on. IEEE, 2015.
