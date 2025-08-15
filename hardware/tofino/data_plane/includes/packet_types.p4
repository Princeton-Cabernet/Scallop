#pragma once

typedef bit<8> packet_type_t;

const packet_type_t PACKET_TYPE_CONSUME_SFU = 0x01;
const packet_type_t PACKET_TYPE_CONSUME_CLIENTS_HOST = 0x02;
const packet_type_t PACKET_TYPE_CONSUME_CONTROLLER = 0x03;
const packet_type_t PACKET_TYPE_CONSUME_FORWARDED = 0x04;
const packet_type_t PACKET_TYPE_CONSUME_CPU = 0x05;
const packet_type_t PACKET_TYPE_CONSUME_BLUEFIELD_P0 = 0x06;
const packet_type_t PACKET_TYPE_CONSUME_BLUEFIELD_P1 = 0x07;

const packet_type_t PACKET_TYPE_SFU_ARP  = 0x11;
const packet_type_t PACKET_TYPE_SFU_ICMP = 0x12;
const packet_type_t PACKET_TYPE_SFU_UDP  = 0x13;
const packet_type_t PACKET_TYPE_SFU_TCP  = 0x14;
const packet_type_t PACKET_TYPE_CPU_INCOMING  = 0x15;
const packet_type_t PACKET_TYPE_APP_REPLICATE = 0x16;
const packet_type_t PACKET_TYPE_APP_FORWARD   = 0x17;

#define RTCP_SR 200
#define RTCP_RR 201
#define RTCP_NACK 205
#define RTCP_PLI 206

const packet_type_t AV1_LAYER_DROP = 0x21;
