<p align="left">
  <img src="doc/img/scallop-readme.png" alt="Scallop SFU" width="180"/>
</p>

[![build/test controller/agent](https://github.com/Princeton-Cabernet/scallop/actions/workflows/build-test-controller-agent.yml/badge.svg)](https://github.com/Princeton-Cabernet/scallop/actions/workflows/build-test-controller-agent.yml)
[![build/test client](https://github.com/Princeton-Cabernet/scallop/actions/workflows/build-test-client.yml/badge.svg)](https://github.com/Princeton-Cabernet/scallop/actions/workflows/build-test-client.yml)

**Abstract.** This project rethinks the key building block for video-conferencing infrastructures - selective forwarding units (SFUs). SFUs relay and adapt media streams between participants and, today, run in software on general-purpose servers. Our main insight, discerned from dissecting the operation of production SFU servers, is that SFUs largely mimic traditional packet-processing operations such as dropping and forwarding. Guided by this, we present **Scallop**, an SDN-inspired SFU that decouples video-conferencing applications into a hardware-based data plane for latency-sensitive and frequent media operations, and a software control plane for the (infrequent) remaining tasks, such as analyzing feedback signals and session management. Scallop is a general design that is suitable for a variety of hardware platforms, including programmable switches and SmartNICs. Our Tofino-based implementation fully supports WebRTC and delivers 7-422x improved scaling over a 32-core commodity server, while reaping performance improvements by cutting forwarding-induced latency by 26x.


**Paper.** The code in this repository supports the research presented in our paper that provides a comprehensive introduction to Scallop: 

> Oliver Michel, Satadal Sengupta, Hyojoon Kim, Ravi Netravali, and Jennifer Rexford. 2025. Scalable Video Conferencing Using SDN Principles. In ACM SIGCOMM 2025 Conference (SIGCOMM ’25), September 8–11, 2025, Coimbra, Portugal. ACM, New York, NY, USA, 19 pages. https://doi.org/10.1145/3718958.3750489

The paper is also available [here](scallop.pdf).


## Building the code

### Controller, Switch Agent, Data-Plane Model

```shell
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
```

### Client Application

```shell
cd client
npm install
npm run build
```

## Create certificate

* for *localhost*:

```shell
openssl req -x509 -newkey rsa:4096 -keyout key.pem -out cert.pem \
  -days 365 -nodes -subj "/CN=localhost"
```

## Run Scallop

Scallop primarily runs on Intel Tofino-based programmable switches. We provide instructions for running Scallop on a Tofino switch below. Alternatively, Scallop also comes with a software data-plane implementation that runs on a commodity server. The software mimics the internals and behavior of the Tofino data plane as closely as possible.

### Controller

```shell
build/controller/controller -d <data_plane_ip:port>
```

### Tofino Agent

```shell
build/agent/tofino_agent -l <veth>
```

### Tofino PRE Adapter

The following version of the command works with the software data-plane implementation:
```shell
hardware/tofino/control_plane/tofino_interface.py
```

When deploying the actual Tofino P4 implementation, use this version of the command instead:
```shell
hardware/tofino/control_plane/tofino_interface.py --hardware
```

### Software Model

```shell
build/model/model
```

### Client

```shell
client/run_browser_macos.sh
```
or
```shell
client/run_browser_ubuntu.sh
```

### Hardware Prototypes

The hardware prototypes described and evaluated in the paper are available under `hardware/`: The Tofino P4 code is available in `hardware/tofino/data_plane` and the Bluefield-3 P4 code is available in `hardware/bluefield`.

## License

This project's source code is released under the [GNU Affero General Public License v3](https://www.gnu.org/licenses/agpl-3.0.html). In particular,
* You are entitled to redistribute the program or its modified version, however you must also make available the full source code and a copy of the license to the recipient. Any modified version or derivative work must also be licensed under the same licensing terms.
* You also must make available a copy of the modified program's source code available, under the same licensing terms, to all users interacting with the modified program remotely through a computer network.
