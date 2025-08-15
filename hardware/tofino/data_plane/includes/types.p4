#pragma once

typedef bit<48> mac_addr_t;
typedef bit<32> ipv4_addr_t;
typedef bit<16> tp_port_t;

typedef bit<16> ether_type_t;
const ether_type_t ETHERTYPE_IPV4 = 16w0x0800;
const ether_type_t ETHERTYPE_ARP  = 16w0x0806;

// ARP types
const bit<16> ARP_HTYPE   = 0x0001; // Ethernet Hardware type is 1
const bit<16> ARP_PTYPE   = ETHERTYPE_IPV4; // Protocol used for ARP is IPV4
const bit<8>  ARP_HLEN    = 6; // Ethernet address size is 6 bytes
const bit<8>  ARP_PLEN    = 4; // IP address size is 4 bytes
const bit<16> ARP_REQUEST = 1; // Operation 1 is request
const bit<16> ARP_REPLY   = 2; // Operation 2 is reply

typedef bit<8> ip_protocol_t;
const ip_protocol_t IP_PROTOCOL_ICMP = 1;
const ip_protocol_t IP_PROTOCOL_TCP  = 6;
const ip_protocol_t IP_PROTOCOL_UDP  = 17;

typedef bit<16> L1ExclusionId__t;
typedef bit<9>  L2ExclusionId__t;
typedef bit<16> ReplicationId__t;

typedef bit<32> RcvStreamSignature_t;
typedef bit<8>  RcvStreamDataWidth_t;

typedef bit<16> Counter16_t;
typedef bit<32> Counter32_t;