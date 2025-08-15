/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
 * SPDX-License-Identifier: LicenseRef-NvidiaProprietary
 *
 * NVIDIA CORPORATION, its affiliates and licensors retain all intellectual
 * property and proprietary rights in and to this material, related
 * documentation and any modifications thereto. Any use, reproduction,
 * disclosure or distribution of this material and related documentation
 * without an express license agreement from NVIDIA CORPORATION or
 * its affiliates is strictly prohibited.
 */

#include <doca_model.p4>
#include <doca_headers.p4>
#include <doca_externs.p4>
#include <doca_parser.p4>

/*
 * This program implements Scallop, a system for scalable network support for video conferencing.
 */

// Logical ports
const nv_logical_port_t P0_PORT  = 0;
const nv_logical_port_t P1_PORT  = 1; // Only works together with p0 when multiswitch mode is on
const nv_logical_port_t P0_HPF   = 10;
const nv_logical_port_t P1_HPF   = 11;
const nv_logical_port_t P0_SF_02 = 22;
const nv_logical_port_t P0_SF_03 = 23;

// Loopback (logical) ports
const nv_logical_port_t LOOPBACK_TEMPLATE_PORT = P0_PORT;
const nv_logical_port_t LOOPBACK_MIRROR_PORT   = P1_PORT;

// SFU properties
const nv_ipv4_addr_t BF3_SFU_IPV4_ADDR = 0x0a00d3de;     // 10.0.211.222
const nv_mac_addr_t  BF3_SFU_MAC_ADDR  = 0xa088c2a8d846; // SFU MAC address

// Controller properties
const nv_ipv4_addr_t CABERNET802_IP  = 0x0a00d301;     // 10.0.211.1
const nv_mac_addr_t  CABERNET802_MAC = 0x506b4bc40191; // enp134s0f1np1 (host)

// Client properties
const nv_ipv4_addr_t CABMEATY02_IP  = 0x0a00d302;     // 10.0.211.2
const nv_mac_addr_t  CABMEATY02_MAC = 0x1c34da7587f3; // eno34np1 (host)
const bit<16> RECEIVER_1_PORT = 47510;
const bit<16> RECEIVER_2_PORT = 47520;
const bit<16> RECEIVER_3_PORT = 47530;
const bit<16> RECEIVER_4_PORT = 47540;
const bit<16> RECEIVER_5_PORT = 47550;
const bit<16> RECEIVER_6_PORT = 47560;

struct metadata_t {
    bit<32> meeting_id;
    bit<16> replica_id;
    bit<16> replica_bits;
    nv_ipv4_addr_t receiver_ip;
    bit<16> receiver_port;
    nv_mac_addr_t receiver_mac;
    bit<1> receiver_exists;
    bit<1> is_sender_replica_matched;
    bit<1> does_tree_height_match;
}

// UDP ports used for parsing and replication
#define SFU_UDP_PORT 3000
#define STUN_TEMP_PORT 3001
#define RTP_TEMP_PORT 3002
#define RTCP_TEMP_PORT 3003
#define RTP_W_EXTN_TEMP_PORT 3004
#define REPLICA_UDP_PORT 3005

header stun_h {
    bit<2>  zeros;
    bit<14> msg_type;
    bit<16> msg_len;
    bit<32> magic_cookie;
    bit<96> transaction_id;
}

header rtp_h {
    bit<2>  version;
    bit<1>  padding;
    bit<1>  extension;
    bit<4>  csrc_count;
    bit<1>  marker;
    bit<7>  payload_type;
    bit<16> sequence_num;
    bit<32> timestamp;
    bit<32> ssrc;
}

header rtcp_h {
    bit<2>  version;
    bit<1>  padding;
    bit<5>  reception_report_count;
    bit<8>  packet_type;
    bit<16> rtcp_length;
    bit<32> sender_ssrc;
}

header post_rtp_h {
    bit<16> extension_type;
    bit<16> extension_length;
}

struct av1_extension_t {
    bit<1>  frame_start;
    bit<1>  frame_end;
    bit<6>  dep_template_id;
    bit<16> frame_num;
}

header rtp_extensions_onebyteheadertype_h {
    bit<32> first_extension;
    bit<16> second_extension;
    bit<8>  third_extension_header;
    av1_extension_t av1;
    bit<16> data;
}

header rtp_extensions_twobyteheadertype_h {
    bit<40> first_extension;
    bit<24> second_extension;
    bit<16> third_extension_header;
    av1_extension_t av1;
    bit<24> data;
}

// Combined STUN, RTP, and RTCP headers for disambiguation
header post_udp_h {
    bit<2> version; // STUN=0 or RTP/RTCP=2
    bit<1> padding;
    bit<1> rtp_extn_rtcp_rrc1;
    bit<4> rtp_csrc_rtcp_rrc2;
    bit<8> rtp_mrkrpltp_rtcp_pt; // marker + payload type (RTP) or packet type (RTCP)
    bit<16> rtp_seqnum_rtcp_len; // RTP sequence number or RTCP length
}

struct headers_t {
    NV_FIXED_HEADERS
    post_udp_h  post_udp; // Common header for STUN, RTP, and RTCP
    stun_h      stun;
    rtp_h       rtp;
    rtcp_h      rtcp;
    rtp_h       rtp_w_extn;
    post_rtp_h  post_rtp;
    rtp_extensions_onebyteheadertype_h   rtp_ext_1b;
    rtp_extensions_twobyteheadertype_h   rtp_ext_2b;
}

parser packet_parser(packet_in packet, out headers_t headers) {
    NV_FIXED_PARSER(packet, headers)

    @nv_transition_from("nv_parse_udp", SFU_UDP_PORT)
    state parse_post_udp {
        packet.extract(headers.post_udp);
        transition accept;
    }

    @nv_transition_from("nv_parse_udp", STUN_TEMP_PORT)
    state parse_stun {
        packet.extract(headers.stun);
        transition accept;
    }

    @nv_transition_from("nv_parse_udp", RTP_TEMP_PORT)
    state parse_rtp {
        packet.extract(headers.rtp);
        transition accept;
    }

    @nv_transition_from("nv_parse_udp", RTCP_TEMP_PORT)
    state parse_rtcp {
        packet.extract(headers.rtcp);
        transition accept;
    }

    @nv_transition_from("nv_parse_udp", RTP_W_EXTN_TEMP_PORT)
    state parse_rtp_with_extensions {
        packet.extract(headers.rtp_w_extn);
        transition parse_post_rtp;
    }

    state parse_post_rtp {
        packet.extract(headers.post_rtp);
        transition select (headers.post_rtp.extension_type) {
            0xbede: parse_onebyteheadertype_extensions; // One-byte header type
            0x1000: parse_twobyteheadertype_extensions; // Two-byte header type
            default: accept;
        }
    }

    state parse_onebyteheadertype_extensions {
        packet.extract(headers.rtp_ext_1b);
        transition accept;
    }

    state parse_twobyteheadertype_extensions {
        packet.extract(headers.rtp_ext_2b);
        transition accept;
    }
}

/**
 * This control admits GTP packets only if the tunnel ID matches
 *
 */
 control scallop(
    inout headers_t headers,
    in nv_standard_metadata_t std_meta,
    inout metadata_t user_meta,
    inout nv_empty_metadata_t pkt_out_meta
) {
    NvCounter(8, NvCounterType.PACKETS_AND_BYTES) hdr_counter;
    NvCounter(4, NvCounterType.PACKETS_AND_BYTES) port_counter;

    action initialize_metadata() {
        user_meta.meeting_id = 0;
        user_meta.replica_id = 0;
        user_meta.replica_bits = 0;
        user_meta.receiver_ip = 0;
        user_meta.receiver_port = 0;
        user_meta.receiver_mac = 0;
        user_meta.receiver_exists = 0;
        user_meta.is_sender_replica_matched = 0;
        user_meta.does_tree_height_match = 0;
    }

    action reparse_as_stun() {
        nv_set_l4_dst_port(headers, STUN_TEMP_PORT);
        nv_force_reparse(headers);
    }

    action reparse_as_rtp() {
        nv_set_l4_dst_port(headers, RTP_TEMP_PORT);
        nv_force_reparse(headers);
    }

    action reparse_as_rtcp() {
        nv_set_l4_dst_port(headers, RTCP_TEMP_PORT);
        nv_force_reparse(headers);
    }

    action reparse_as_rtp_with_extensions() {
        nv_set_l4_dst_port(headers, RTP_W_EXTN_TEMP_PORT);
        nv_force_reparse(headers);
    }

    table post_udp_table {
        key = {
            headers.post_udp.version: exact;
            headers.post_udp.rtp_mrkrpltp_rtcp_pt: ternary;
            headers.post_udp.rtp_extn_rtcp_rrc1: ternary;
        }
        actions = {
            reparse_as_stun;
            reparse_as_rtp;
            reparse_as_rtcp;
            reparse_as_rtp_with_extensions;
            NoAction;
        }
        default_action = NoAction();
        const entries = {
            (0, _, _)   : reparse_as_stun();
            (2, 200, _) : reparse_as_rtcp();
            (2, 201, _) : reparse_as_rtcp();
            (2, 202, _) : reparse_as_rtcp();
            (2, 203, _) : reparse_as_rtcp();
            (2, 204, _) : reparse_as_rtcp();
            (2, 205, _) : reparse_as_rtcp();
            (2, 206, _) : reparse_as_rtcp();
            (2, 207, _) : reparse_as_rtcp();
            (2, _, 0)   : reparse_as_rtp();
            (2, _, 1)   : reparse_as_rtp_with_extensions();
        }
    }

    // Sender to meeting map
    
    action retrieve_meeting_id(bit<32> meeting_id) {
        user_meta.meeting_id = meeting_id;
    }

    table sender_meeting_map {
        key = {
            headers.ipv4.src_addr: exact;
            headers.udp.src_port:  exact;
        }
        actions = {
            retrieve_meeting_id;
            NoAction;
        }
        default_action = NoAction();
        const entries = {
            (CABMEATY02_IP,  RECEIVER_1_PORT): retrieve_meeting_id(0x1);
            (CABMEATY02_IP,  RECEIVER_2_PORT): retrieve_meeting_id(0x1);
            (CABMEATY02_IP,  RECEIVER_3_PORT): retrieve_meeting_id(0x1);
            (CABMEATY02_IP,  RECEIVER_4_PORT): retrieve_meeting_id(0x1);
            (CABMEATY02_IP,  RECEIVER_5_PORT): retrieve_meeting_id(0x1);
            (CABMEATY02_IP,  RECEIVER_6_PORT): retrieve_meeting_id(0x1);
        }
    }

    // Sender-replica match

    action retrieve_sender_replica_match(bit<1> match) {
        user_meta.is_sender_replica_matched = match;
    }

    table sender_replica_match {
        key = {
            user_meta.meeting_id: exact;
            headers.ipv4.src_addr: exact;
            headers.udp.src_port: exact;
            user_meta.replica_id: exact;
        }
        actions = {
            retrieve_sender_replica_match;
            NoAction;
        }
        default_action = NoAction();
        const entries = {
            (0x1, CABMEATY02_IP, RECEIVER_1_PORT, 0x1): retrieve_sender_replica_match(1);
            (0x1, CABMEATY02_IP, RECEIVER_2_PORT, 0x2): retrieve_sender_replica_match(1);
            (0x1, CABMEATY02_IP, RECEIVER_3_PORT, 0x3): retrieve_sender_replica_match(1);
            (0x1, CABMEATY02_IP, RECEIVER_4_PORT, 0x4): retrieve_sender_replica_match(1);
            (0x1, CABMEATY02_IP, RECEIVER_5_PORT, 0x5): retrieve_sender_replica_match(1);
            (0x1, CABMEATY02_IP, RECEIVER_6_PORT, 0x6): retrieve_sender_replica_match(1);
        }
    }

    // Replica to receiver map

    action retrieve_dst_addrs(bit<16> receiver_port, nv_ipv4_addr_t receiver_ip, nv_mac_addr_t receiver_mac) {
        user_meta.receiver_port = receiver_port;
        user_meta.receiver_ip = receiver_ip;
        user_meta.receiver_mac = receiver_mac;
        user_meta.receiver_exists = 1;
    }

    table replica_receiver_map {
        key = {
            user_meta.meeting_id: exact;
            user_meta.replica_id: exact;
        }
        actions = {
            retrieve_dst_addrs;
            NoAction;
        }
        default_action = NoAction();
        const entries = {
            (0x1, 0x1) : retrieve_dst_addrs(RECEIVER_1_PORT, CABMEATY02_IP, CABMEATY02_MAC);
            (0x1, 0x2) : retrieve_dst_addrs(RECEIVER_2_PORT, CABMEATY02_IP, CABMEATY02_MAC);
            (0x1, 0x3) : retrieve_dst_addrs(RECEIVER_3_PORT, CABMEATY02_IP, CABMEATY02_MAC);
            (0x1, 0x4) : retrieve_dst_addrs(RECEIVER_4_PORT, CABMEATY02_IP, CABMEATY02_MAC);
            (0x1, 0x5) : retrieve_dst_addrs(RECEIVER_5_PORT, CABMEATY02_IP, CABMEATY02_MAC);
            (0x1, 0x6) : retrieve_dst_addrs(RECEIVER_6_PORT, CABMEATY02_IP, CABMEATY02_MAC);
        }
    }

    // Tree height match

    action retrieve_tree_height_match(bit<1> match) {
        user_meta.does_tree_height_match = match;
    }

    table meeting_tree_height_match {
        key = {
            user_meta.meeting_id: exact;
            user_meta.replica_bits: exact;
        }
        actions = {
            retrieve_tree_height_match;
            NoAction;
        }
        default_action = NoAction();
        const entries = {
            (0x1, 0x3): retrieve_tree_height_match(1); // Height 3
        }
    }

    action read_replica_id_and_bits() {
        user_meta.replica_id = headers.ipv4.dst_addr[15:0];
        user_meta.replica_bits = headers.ipv4.dst_addr[31:16];
    }

    action write_replica_id_and_bits() {
        headers.ipv4.dst_addr[15:0] = user_meta.replica_id;
        headers.ipv4.dst_addr[31:16] = user_meta.replica_bits;
    }

    action increment_replica_id() {
        user_meta.replica_id += 1;
    }

    action increment_replica_bits() {
        user_meta.replica_bits += 1;
    }

    action set_src_addrs() {
        nv_set_l4_src_port(headers, SFU_UDP_PORT);
        headers.ipv4.src_addr = BF3_SFU_IPV4_ADDR;
        headers.ethernet.src_addr = BF3_SFU_MAC_ADDR;
    }

    action set_dst_addrs() {
        nv_set_l4_dst_port(headers, user_meta.receiver_port);
        headers.ipv4.dst_addr = user_meta.receiver_ip;
        headers.ethernet.dst_addr = user_meta.receiver_mac;
    }

    action append_bit_to_replica_id_left() {
        // Append 0 to the end of the replica_id
        user_meta.replica_id = (user_meta.replica_id << 1);
    }

    action append_bit_to_replica_id_right() {
        // Append 1 to the end of the replica_id
        user_meta.replica_id = (user_meta.replica_id << 1);
        user_meta.replica_id += 1;
    }

    apply {
        initialize_metadata();

        if (headers.udp.isValid())        { hdr_counter.count(0); }
        if (headers.post_udp.isValid())   { hdr_counter.count(1); }

        if (headers.post_udp.isValid()) {
            post_udp_table.apply();
        }

        if (headers.stun.isValid())       { hdr_counter.count(2); }
        if (headers.rtp.isValid())        { hdr_counter.count(3); }
        if (headers.rtcp.isValid())       { hdr_counter.count(4); }
        if (headers.post_rtp.isValid())   { hdr_counter.count(5); }
        if (headers.rtp_ext_1b.isValid()) { hdr_counter.count(6); }
        if (headers.rtp_ext_2b.isValid()) { hdr_counter.count(7); }

        if (headers.rtp_ext_1b.isValid() || headers.rtp_ext_2b.isValid()) {
            nv_set_l4_dst_port(headers, REPLICA_UDP_PORT);
            headers.ipv4.dst_addr = 0;
            nv_mirror(LOOPBACK_TEMPLATE_PORT, LOOPBACK_MIRROR_PORT);
        }

        if (headers.udp.isValid() && headers.udp.dst_port == REPLICA_UDP_PORT) {
            if (std_meta.ingress_port == LOOPBACK_TEMPLATE_PORT) { port_counter.count(0); }
            if (std_meta.ingress_port == LOOPBACK_MIRROR_PORT)   { port_counter.count(1); }
        }

        // Implement replication logic
        if (headers.udp.isValid() && headers.udp.dst_port == REPLICA_UDP_PORT) {
            sender_meeting_map.apply();
            read_replica_id_and_bits();
            if (std_meta.ingress_port == LOOPBACK_TEMPLATE_PORT) {
                // Original copy
                append_bit_to_replica_id_left();
            } else if (std_meta.ingress_port == LOOPBACK_MIRROR_PORT) {
                // Mirrored copy
                append_bit_to_replica_id_right();
            }
            increment_replica_bits();
            meeting_tree_height_match.apply();

            // Final tree height, send replica if it matches, mirror otherwise
            if (user_meta.does_tree_height_match == 1) {
                increment_replica_id();
                replica_receiver_map.apply();
                sender_replica_match.apply();
                if (user_meta.receiver_exists == 1
                        && user_meta.is_sender_replica_matched == 0) {
                    set_src_addrs();
                    set_dst_addrs();
                    nv_send_to_port(P0_PORT);
                }
            
            } else {
                write_replica_id_and_bits();
                nv_mirror(LOOPBACK_TEMPLATE_PORT, LOOPBACK_MIRROR_PORT);
            }
        }
    }
}

NvDocaPipeline(
    packet_parser(),
    scallop()
) main;