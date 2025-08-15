#pragma once

parser TofinoIngressParser(
        packet_in pkt,
        out ingress_intrinsic_metadata_t ig_intr_md) {
    
    state start {
        pkt.extract(ig_intr_md);
        transition select(ig_intr_md.resubmit_flag) {
            1: parse_resubmit;
            0: parse_port_metadata;
        }
    }

    state parse_resubmit {
        // pkt.advance(128); // tofino2 resubmit metadata size
        // transition accept;
        transition reject;
    }
    
    state parse_port_metadata {
        pkt.advance(PORT_METADATA_SIZE);
        transition accept;
    }
}

parser SwitchIngressParser(
        packet_in pkt,
        out header_t hdr,
        out ig_metadata_t ig_md,
        out ingress_intrinsic_metadata_t ig_intr_md) {

    TofinoIngressParser() tofino_parser;

    ParserCounter() rtp_extn_len_counter;
    Checksum() icmp_checksum;

    state start {
        tofino_parser.apply(pkt, ig_intr_md);
        transition select (pkt.lookahead<null_loopback_h>().reserved) {
            0x02000000: parse_null_loopback; // For our test trace
            default   : parse_ethernet;
        }
    }

    state parse_null_loopback {
        pkt.extract(hdr.null_loopback);
        transition parse_ipv4_conditional;
    }

    state parse_ethernet {
        pkt.extract(hdr.ethernet);
        transition select (hdr.ethernet.ether_type) {
            ETHERTYPE_ARP : parse_arp;
            ETHERTYPE_IPV4: parse_ipv4_conditional;
            default: accept;
        }
    }

    state parse_arp {
        pkt.extract(hdr.arp);
        transition accept;
    }

    state parse_ipv4_conditional {
        transition parse_ipv4_check_srcaddr;
    }

    state parse_ipv4_check_srcaddr {
        transition select (pkt.lookahead<ipv4_h>().src_addr) {
            SFU_IPV4_ADDR: parse_ipv4;
            BF3_SFU_IPV4_ADDR: parse_ipv4;
            CABMEATY02_IP: parse_ipv4;
            CABERNET802_IP: parse_ipv4;
            default: parse_ipv4_check_dstaddr;
        }
    }

    state parse_ipv4_check_dstaddr {
        transition select (pkt.lookahead<ipv4_h>().dst_addr) {
            SFU_IPV4_ADDR: parse_ipv4;
            BF3_SFU_IPV4_ADDR: parse_ipv4;
            CABMEATY02_IP: parse_ipv4;
            CABERNET802_IP: parse_ipv4;
            default: parse_ipv4_then_stop;
        }
    }

    state parse_ipv4_then_stop {
        pkt.extract(hdr.ipv4);
        transition accept;
    }

    state parse_ipv4 {
        pkt.extract(hdr.ipv4);
        transition select (hdr.ipv4.protocol) {
            IP_PROTOCOL_ICMP: parse_icmp;
            IP_PROTOCOL_UDP : parse_udp;
            IP_PROTOCOL_TCP : parse_tcp;
            // default: reject;
            default: accept;
        }
    }

    state parse_icmp {
        pkt.extract(hdr.icmp);
        transition accept;
    }

    state parse_tcp {
        pkt.extract(hdr.tcp);
        transition accept;
    }

    state parse_udp {
        pkt.extract(hdr.udp);
        transition select (hdr.udp.udp_total_len) {
            // 0..15  : reject; // Too short for RTP/RTCP
            0..15  : accept;
            default: try_parse_stun_rtp_rtcp_conditional;
        }
    }

    state try_parse_stun_rtp_rtcp_conditional {
        transition select (hdr.udp.src_port, hdr.udp.dst_port) {
            (SFU_UDP_PORT, _): try_parse_stun_rtp_rtcp;
            (_, SFU_UDP_PORT): try_parse_stun_rtp_rtcp;
            default: accept;
        }
    }

    state try_parse_stun_rtp_rtcp {
        transition select (
            pkt.lookahead<stun_rtp_rtcp_h>().version,
            pkt.lookahead<stun_rtp_rtcp_h>().rtp_mrkrpltp_rtcp_pt,
            pkt.lookahead<stun_rtp_rtcp_h>().rtp_extn_rtcp_rrc1
        ) {
            (0, _, _): parse_stun;
            (2, 200..207, _): parse_rtcp;
            (2, _, 1): parse_rtp_with_extns;
            (2, _, 0): parse_rtp_wout_extns;
            default: accept;
        }
    }

    state parse_stun {
        pkt.extract(hdr.stun);
        transition accept;
    }

    state parse_rtp {
        pkt.extract(hdr.rtp);
        transition accept;
    }

    state parse_rtp_with_extns {
        pkt.extract(hdr.rtp);
        transition parse_rtp_extensions;
    }

    state parse_rtp_wout_extns {
        pkt.extract(hdr.rtp);
        transition accept;
    }

    state parse_rtcp {
        pkt.extract(hdr.rtcp);
        transition select (hdr.rtcp.packet_type) {
            RTCP_SR  : accept; // SR
            RTCP_RR  : accept; // RR/REMB
            RTCP_NACK: accept; // NACK
            RTCP_PLI : accept; // PLI
            default  : accept;
        }
    }

    state parse_rtp_extensions {
        // Set to LSB, rotate it by 6 bits to multiply by 4, mask by 2^(6+1) - 1 to remove sign bit
        rtp_extn_len_counter.set(pkt.lookahead<rtpextn_onebytetype_twobytetype_h>().extn_len_lsb, 255, 6, 6, 0); // Set counter to length in bytes
        transition select (
            pkt.lookahead<rtpextn_onebytetype_twobytetype_h>().onebyte_be_twobyte_10,
            pkt.lookahead<rtpextn_onebytetype_twobytetype_h>().onebyte_d_twobyte_0,
            pkt.lookahead<rtpextn_onebytetype_twobytetype_h>().onebyte_e_twobyte_appbits,
            pkt.lookahead<rtpextn_onebytetype_twobytetype_h>().extn_len_msb
        ) {
            (0xbe, 0xd, 0xe, 0): parse_rtp_extn_onebytehdrtype;
            (0x10, 0x0, _, 0):   parse_rtp_extn_twobytehdrtype;
            default: accept; // Not a valid RTP extension header or the RTP extension header is too long
        }
    }

    state parse_rtp_extn_onebytehdrtype {
        pkt.extract(hdr.rtp_extn_onebytehdr);
        transition parse_rtp_extn_onebytehdrtype_element_1;
    }

    state parse_rtp_extn_twobytehdrtype {
        pkt.extract(hdr.rtp_extn_twobytehdr);
        transition parse_rtp_extn_twobytehdrtype_element_1;
    }

    // Element 1 header: 1-byte header type
    state parse_rtp_extn_onebytehdrtype_element_1 {
        pkt.extract(hdr.rtp_extn_elem_onebytehdr_1);
        rtp_extn_len_counter.decrement(1);
        transition select (
            hdr.rtp_extn_elem_onebytehdr_1.id,
            hdr.rtp_extn_elem_onebytehdr_1.len
        ) {
            (4w0, _):       accept; // Invalid ID, don't parse further (according to RFC 8285)
            (4w15, _):      accept; // Stop parsing immediately (according to RFC 8285)
            (4w12, 0..1):   accept; // Found AV1 but descriptor too short
            (4w12, _):      parse_av1;
            (_, 0):         parse_onebytehdrtype_element_1_onebytedata;
            (_, 2):         parse_onebytehdrtype_element_1_threebytedata;
            (_, 14):        parse_onebytehdrtype_element_1_fifteenbytedata;
            default:        accept;
        }
    }

    // Element 1 header: 2-byte header type
    state parse_rtp_extn_twobytehdrtype_element_1 {
        pkt.extract(hdr.rtp_extn_elem_twobytehdr_1);
        rtp_extn_len_counter.decrement(2);
        transition select (
            hdr.rtp_extn_elem_twobytehdr_1.id,
            hdr.rtp_extn_elem_twobytehdr_1.len
        ) {
            (8w0, _):       accept; // Invalid ID, don't parse further (according to RFC 8285)
            (8w12, 0..1):   accept; // Found AV1 but descriptor too short
            (8w12, _):      parse_av1;
            (_, 1):         parse_twobytehdrtype_element_1_onebytedata;
            (_, 3):         parse_twobytehdrtype_element_1_threebytedata;
            (_, 20):        parse_twobytehdrtype_element_1_twentybytedata;
            default:        accept;
        }
    }

    // Element 2 header: 1-byte header type
    state parse_rtp_extn_onebytehdrtype_element_2 {
        pkt.extract(hdr.rtp_extn_elem_onebytehdr_2);
        rtp_extn_len_counter.decrement(1);
        transition select (
            hdr.rtp_extn_elem_onebytehdr_2.id,
            hdr.rtp_extn_elem_onebytehdr_2.len
        ) {
            (4w0, _):       accept; // Invalid ID, don't parse further (according to RFC 8285)
            (4w15, _):      accept; // Stop parsing immediately (according to RFC 8285)
            (4w12, 0..1):   accept; // Found AV1 but descriptor too short
            (4w12, _):      parse_av1;
            (_, 0):         parse_onebytehdrtype_element_2_onebytedata;
            (_, 2):         parse_onebytehdrtype_element_2_threebytedata;
            (_, 14):        parse_onebytehdrtype_element_2_fifteenbytedata;
            default:        accept;
        }
    }

    // Element 2 header: 2-byte header type
    state parse_rtp_extn_twobytehdrtype_element_2 {
        pkt.extract(hdr.rtp_extn_elem_twobytehdr_2);
        rtp_extn_len_counter.decrement(2);
        transition select (
            hdr.rtp_extn_elem_twobytehdr_2.id,
            hdr.rtp_extn_elem_twobytehdr_2.len
        ) {
            (8w0, _):       accept; // Invalid ID, don't parse further (according to RFC 8285)
            (8w12, 0..1):   accept; // Found AV1 but descriptor too short
            (8w12, _):      parse_av1;
            (_, 1):         parse_twobytehdrtype_element_2_onebytedata;
            (_, 3):         parse_twobytehdrtype_element_2_threebytedata;
            (_, 20):        parse_twobytehdrtype_element_2_twentybytedata;
            default:        accept;
        }
    }

    // Element 3 header: 1-byte header type
    state parse_rtp_extn_onebytehdrtype_element_3 {
        pkt.extract(hdr.rtp_extn_elem_onebytehdr_3);
        rtp_extn_len_counter.decrement(1);
        transition select (
            hdr.rtp_extn_elem_onebytehdr_3.id,
            hdr.rtp_extn_elem_onebytehdr_3.len
        ) {
            (4w0, _):       accept; // Invalid ID, don't parse further (according to RFC 8285)
            (4w15, _):      accept; // Stop parsing immediately (according to RFC 8285)
            (4w12, 0..1):   accept; // Found AV1 but descriptor too short
            (4w12, _):      parse_av1;
            (_, 0):         parse_onebytehdrtype_element_3_onebytedata;
            (_, 2):         parse_onebytehdrtype_element_3_threebytedata;
            (_, 14):        parse_onebytehdrtype_element_3_fifteenbytedata;
            default:        accept;
        }
    }

    // Element 3 header: 2-byte header type
    state parse_rtp_extn_twobytehdrtype_element_3 {
        pkt.extract(hdr.rtp_extn_elem_twobytehdr_3);
        rtp_extn_len_counter.decrement(2);
        transition select (
            hdr.rtp_extn_elem_twobytehdr_3.id,
            hdr.rtp_extn_elem_twobytehdr_3.len
        ) {
            (8w0, _):       accept; // Invalid ID, don't parse further (according to RFC 8285)
            (8w12, 0..1):   accept; // Found AV1 but descriptor too short
            (8w12, _):      parse_av1;
            (_, 1):         parse_twobytehdrtype_element_3_onebytedata;
            (_, 3):         parse_twobytehdrtype_element_3_threebytedata;
            (_, 20):        parse_twobytehdrtype_element_3_twentybytedata;
            default:        accept;
        }
    }

    // Element 1, 1-byte header type, 1 byte data
    state parse_onebytehdrtype_element_1_onebytedata {
        pkt.extract(hdr.rtp_extn_elem_onebytehdr_1_onebytedata);
        rtp_extn_len_counter.decrement(1);
        transition select (rtp_extn_len_counter.is_zero()) {
            true: accept;
            default: parse_rtp_extn_onebytehdrtype_element_2;
        }
    }

    // Element 1, 2-byte header type, 1 byte data
    state parse_twobytehdrtype_element_1_onebytedata {
        pkt.extract(hdr.rtp_extn_elem_twobytehdr_1_onebytedata);
        rtp_extn_len_counter.decrement(1);
        transition select (rtp_extn_len_counter.is_zero()) {
            true: accept;
            default: parse_rtp_extn_twobytehdrtype_element_2;
        }
    }

    // Element 1, 1-byte header type, 3 byte data
    state parse_onebytehdrtype_element_1_threebytedata {
        pkt.extract(hdr.rtp_extn_elem_onebytehdr_1_threebytedata);
        rtp_extn_len_counter.decrement(3);
        transition select (rtp_extn_len_counter.is_zero()) {
            true: accept;
            default: parse_rtp_extn_onebytehdrtype_element_2;
        }
    }

    // Element 1, 2-byte header type, 3 byte data
    state parse_twobytehdrtype_element_1_threebytedata {
        pkt.extract(hdr.rtp_extn_elem_twobytehdr_1_threebytedata);
        rtp_extn_len_counter.decrement(3);
        transition select (rtp_extn_len_counter.is_zero()) {
            true: accept;
            default: parse_rtp_extn_twobytehdrtype_element_2;
        }
    }

    // Element 1, 1-byte header type, 15 byte data
    state parse_onebytehdrtype_element_1_fifteenbytedata {
        pkt.extract(hdr.rtp_extn_elem_onebytehdr_1_fifteenbytedata);
        rtp_extn_len_counter.decrement(15);
        transition select (rtp_extn_len_counter.is_zero()) {
            true: accept;
            default: parse_rtp_extn_onebytehdrtype_element_2;
        }
    }

    // Element 1, 2-byte header type, 20 byte data
    state parse_twobytehdrtype_element_1_twentybytedata {
        pkt.extract(hdr.rtp_extn_elem_twobytehdr_1_twentybytedata);
        rtp_extn_len_counter.decrement(20);
        transition select (rtp_extn_len_counter.is_zero()) {
            true: accept;
            default: parse_rtp_extn_twobytehdrtype_element_2;
        }
    }

    // Element 2, 1-byte header type, 1 byte data
    state parse_onebytehdrtype_element_2_onebytedata {
        pkt.extract(hdr.rtp_extn_elem_onebytehdr_2_onebytedata);
        rtp_extn_len_counter.decrement(1);
        transition select (rtp_extn_len_counter.is_zero()) {
            true: accept;
            default: parse_rtp_extn_onebytehdrtype_element_3;
        }
    }

    // Element 2, 2-byte header type, 1 byte data
    state parse_twobytehdrtype_element_2_onebytedata {
        pkt.extract(hdr.rtp_extn_elem_twobytehdr_2_onebytedata);
        rtp_extn_len_counter.decrement(1);
        transition select (rtp_extn_len_counter.is_zero()) {
            true: accept;
            default: parse_rtp_extn_twobytehdrtype_element_3;
        }
    }
    
    // Element 2, 1-byte header type, 3 byte data
    state parse_onebytehdrtype_element_2_threebytedata {
        pkt.extract(hdr.rtp_extn_elem_onebytehdr_2_threebytedata);
        rtp_extn_len_counter.decrement(3);
        transition select (rtp_extn_len_counter.is_zero()) {
            true: accept;
            default: parse_rtp_extn_onebytehdrtype_element_3;
        }
    }

    // Element 2, 2-byte header type, 3 byte data
    state parse_twobytehdrtype_element_2_threebytedata {
        pkt.extract(hdr.rtp_extn_elem_twobytehdr_2_threebytedata);
        rtp_extn_len_counter.decrement(3);
        transition select (rtp_extn_len_counter.is_zero()) {
            true: accept;
            default: parse_rtp_extn_twobytehdrtype_element_3;
        }
    }

    // Element 2, 1-byte header type, 15 byte data
    state parse_onebytehdrtype_element_2_fifteenbytedata {
        pkt.extract(hdr.rtp_extn_elem_onebytehdr_2_fifteenbytedata);
        rtp_extn_len_counter.decrement(15);
        transition select (rtp_extn_len_counter.is_zero()) {
            true: accept;
            default: parse_rtp_extn_onebytehdrtype_element_3;
        }
    }

    // Element 2, 2-byte header type, 20 byte data
    state parse_twobytehdrtype_element_2_twentybytedata {
        pkt.extract(hdr.rtp_extn_elem_twobytehdr_2_twentybytedata);
        rtp_extn_len_counter.decrement(20);
        transition select (rtp_extn_len_counter.is_zero()) {
            true: accept;
            default: parse_rtp_extn_twobytehdrtype_element_3;
        }
    }

    // Element 3, 1-byte header type, 1 byte data
    state parse_onebytehdrtype_element_3_onebytedata {
        pkt.extract(hdr.rtp_extn_elem_onebytehdr_3_onebytedata);
        rtp_extn_len_counter.decrement(1);
        transition accept;
    }

    // Element 3, 2-byte header type, 1 byte data
    state parse_twobytehdrtype_element_3_onebytedata {
        pkt.extract(hdr.rtp_extn_elem_twobytehdr_3_onebytedata);
        rtp_extn_len_counter.decrement(1);
        transition accept;
    }

    // Element 3, 1-byte header type, 3 byte data
    state parse_onebytehdrtype_element_3_threebytedata {
        pkt.extract(hdr.rtp_extn_elem_onebytehdr_3_threebytedata);
        rtp_extn_len_counter.decrement(3);
        transition accept;
    }

    // Element 3, 2-byte header type, 3 byte data
    state parse_twobytehdrtype_element_3_threebytedata {
        pkt.extract(hdr.rtp_extn_elem_twobytehdr_3_threebytedata);
        rtp_extn_len_counter.decrement(3);
        transition accept;
    }

    // Element 3, 1-byte header type, 15 byte data
    state parse_onebytehdrtype_element_3_fifteenbytedata {
        pkt.extract(hdr.rtp_extn_elem_onebytehdr_3_fifteenbytedata);
        rtp_extn_len_counter.decrement(15);
        transition accept;
    }

    // Element 3, 2-byte header type, 20 byte data
    state parse_twobytehdrtype_element_3_twentybytedata {
        pkt.extract(hdr.rtp_extn_elem_twobytehdr_3_twentybytedata);
        rtp_extn_len_counter.decrement(20);
        transition accept;
    }

    state parse_av1 {
        pkt.extract(hdr.av1);
        transition accept;
    }
}

// Egress parsers

parser TofinoEgressParser(
        packet_in pkt,
        out egress_intrinsic_metadata_t eg_intr_md) {
    
    state start {
        pkt.extract(eg_intr_md);
        transition accept;
    }
}

parser SwitchEgressParser(
        packet_in pkt,
        out header_t hdr,
        out eg_metadata_t eg_md,
        out egress_intrinsic_metadata_t eg_intr_md) {

    TofinoEgressParser() tofino_parser;

    Checksum() udp_checksum;
    
    state start {
        tofino_parser.apply(pkt, eg_intr_md);
        transition parse_bridged_packet_type;
    }

    state parse_bridged_packet_type {
        pkt.extract(eg_md.bridged_md);
        transition parse_ethernet;
    }

    state parse_ethernet {
        pkt.extract(hdr.ethernet);
        transition select (hdr.ethernet.ether_type) {
            ETHERTYPE_IPV4: parse_ipv4;
            default: accept;
        }
    }

    state parse_ipv4 {
        pkt.extract(hdr.ipv4);

        udp_checksum.subtract({hdr.ipv4.src_addr});
        udp_checksum.subtract({hdr.ipv4.dst_addr});
        
        transition select (hdr.ipv4.protocol) {
            IP_PROTOCOL_UDP: parse_udp;
            IP_PROTOCOL_TCP: parse_tcp;
            default: accept;
        }
    }

    state parse_tcp {
        pkt.extract(hdr.tcp);
        transition accept;
    }

    state parse_udp {
        pkt.extract(hdr.udp);

        udp_checksum.subtract({hdr.udp.checksum});
        udp_checksum.subtract({hdr.udp.src_port});
        udp_checksum.subtract({hdr.udp.dst_port});
        eg_md.checksum_udp_temp = udp_checksum.get();

        transition select (hdr.udp.udp_total_len) {
            0..15  : accept;
            default: try_parse_stun_rtp_rtcp;
        }
    }

    state try_parse_stun_rtp_rtcp {
        transition select (
            pkt.lookahead<stun_rtp_rtcp_h>().version,
            pkt.lookahead<stun_rtp_rtcp_h>().rtp_mrkrpltp_rtcp_pt
        ) {
            (0, _): parse_stun;
            (2, 200..207): parse_rtcp;
            (2, _): parse_rtp;
            default: accept;
        }
    }

    state parse_stun {
        pkt.extract(hdr.stun);
        transition accept;
    }

    state parse_rtp {
        pkt.extract(hdr.rtp);
        transition accept;
    }

    state parse_rtcp {
        pkt.extract(hdr.rtcp);
        transition select (hdr.rtcp.packet_type) {
            RTCP_SR  : accept; // SR
            RTCP_RR  : accept; // RR/REMB
            RTCP_NACK: accept; // NACK
            RTCP_PLI : accept; // PLI
            default  : accept;
        }
    }

}