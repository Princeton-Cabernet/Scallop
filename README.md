<p align="left">
  <img src="doc/img/scallop-readme.png" alt="Scallop SFU" width="180"/>
</p>

[![build/test controller/agent](https://github.com/Princeton-Cabernet/scallop/actions/workflows/build-test-controller-agent.yml/badge.svg)](https://github.com/Princeton-Cabernet/scallop/actions/workflows/build-test-controller-agent.yml)
[![build/test client](https://github.com/Princeton-Cabernet/scallop/actions/workflows/build-test-client.yml/badge.svg)](https://github.com/Princeton-Cabernet/scallop/actions/workflows/build-test-client.yml)

**Abstract.** This project rethinks the key building block for video-conferencing infrastructures - selective forwarding units (SFUs). SFUs relay and adapt media streams between participants and, today, run in software on general-purpose servers. Our main insight, discerned from dissecting the operation of production SFU servers, is that SFUs largely mimic traditional packet-processing operations such as dropping and forwarding. Guided by this, we present **Scallop**, an SDN-inspired SFU that decouples video-conferencing applications into a hardware-based data plane for latency-sensitive and frequent media operations, and a software control plane for the (infrequent) remaining tasks, such as analyzing feedback signals and session management. Scallop is a general design that is suitable for a variety of hardware platforms, including programmable switches and SmartNICs. Our Tofino-based implementation fully supports WebRTC and delivers 7-422x improved scaling over a 32-core commodity server, while reaping performance improvements by cutting forwarding-induced latency by 26x.

## Building the code

### Controller and Switch Agent

```shell
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
```

### Data Plane


### Client Application

```shell
cd client
npm install
npm run build
```
