#pragma once

struct ig_metadata_t {
    packet_type_t packet_type;
    bit<1> bypass_egress;
    bit<16> checksum_icmp_temp;
    bit<32> rtp_rtcp_ssrc;
    bit<8> template_id_mod;
    bridged_md_h bridged_md;
}

struct eg_metadata_t {
    bridged_md_h bridged_md;
    packet_type_t packet_type;
    bit<32> rtp_rtcp_ssrc;
    packet_type_t template_id_mod;
    bit<16> checksum_udp_temp;
}