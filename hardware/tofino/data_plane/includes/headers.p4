#pragma once

header bridged_md_h {
    packet_type_t template_id_mod;
}

header null_loopback_h {
    bit<32> reserved;
}

header ethernet_h {
    mac_addr_t dst_addr;
    mac_addr_t src_addr;
    bit<16> ether_type;
}

header arp_t {
    bit<16> h_type;
    bit<16> p_type;
    bit<8>  h_len;
    bit<8>  p_len;
    bit<16> op_code;
    mac_addr_t  src_mac;
    ipv4_addr_t src_ip;
    mac_addr_t  dst_mac;
    ipv4_addr_t dst_ip;
}

header ipv4_h {
    bit<4>  version;
    bit<4>  ihl;
    bit<8>  diffserv;
    bit<16> total_len;
    bit<16> identification;
    bit<3>  flags;
    bit<13> frag_offset;
    bit<8>  ttl;
    bit<8>  protocol;
    bit<16> hdr_checksum;
    ipv4_addr_t src_addr;
    ipv4_addr_t dst_addr;
}

header icmp_h {
    bit<8>  type;
    bit<8>  code;
    bit<16> checksum;
}

header udp_h {
    tp_port_t src_port;
    tp_port_t dst_port;
    bit<16> udp_total_len;
    bit<16> checksum;
}

header tcp_h {
    tp_port_t src_port;
    tp_port_t dst_port;
    bit<32> seq_no;
    bit<32> ack_no;
    bit<4>  data_offset;
    bit<4>  res;
    bit<8>  flags;
    bit<16> window;
    bit<16> checksum;
    bit<16> urgent_ptr;
}

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

header rtp_extn_onebytehdrtype_h {
    bit<16> bede;
    bit<16> extn_len;
}

header rtp_extn_twobytehdrtype_h {
    bit<12> num_100;
    bit<4>  appbits;
    bit<16> extn_len;
}

header rtp_extn_element_onebytehdrtype_h {
    bit<4> id;
    bit<4> len;
}

header rtp_extn_element_twobytehdrtype_h {
    bit<8> id;
    bit<8> len;
}

header rtp_extn_element_onebytehdrtype_onebytedata_h {
    bit<8> data;
}

header rtp_extn_element_onebytehdrtype_threebytedata_h {
    bit<24> data;
}

header rtp_extn_element_onebytehdrtype_fifteenbytedata_h {
    bit<120> data;
}

header rtp_extn_element_twobytehdrtype_onebytedata_h {
    bit<8> data;
}

header rtp_extn_element_twobytehdrtype_threebytedata_h {
    bit<24> data;
}

header rtp_extn_element_twobytehdrtype_twentybytedata_h {
    bit<160> data;
}

header av1_h {
    bit<1>  frame_start;
    bit<1>  frame_end;
    bit<6>  dep_template_id;
    bit<16> frame_num;
}

header rtcp_h {
    bit<2>  version;
    bit<1>  padding;
    bit<5>  reception_report_count;
    bit<8>  packet_type;
    bit<16> rtcp_length;
    bit<32> sender_ssrc;
}

// Combined RTP and RTCP headers for disambiguation

header stun_rtp_rtcp_h {
    bit<2> version; // STUN zeros or RTP/RTCP version
    bit<1> padding;
    bit<1> rtp_extn_rtcp_rrc1;
    bit<4> rtp_csrc_rtcp_rrc2;
    bit<8> rtp_mrkrpltp_rtcp_pt; // marker + payload type (RTP) or packet type (RTCP)
}

// Combined RTP one-byte-header type and two-byte-header type extension headers for disambiguation

header rtpextn_onebytetype_twobytetype_h {
    bit<8> onebyte_be_twobyte_10;
    bit<4> onebyte_d_twobyte_0;
    bit<4> onebyte_e_twobyte_appbits;
    bit<8> extn_len_msb;
    bit<8> extn_len_lsb;
}

// Headers extracted

struct header_t {
    null_loopback_h null_loopback;
    ethernet_h ethernet;
    arp_t   arp;
    ipv4_h  ipv4;
    icmp_h  icmp;
    tcp_h   tcp;
    udp_h   udp;
    stun_h  stun;
    rtp_h   rtp;
    rtcp_h  rtcp;
    
    rtp_extn_onebytehdrtype_h  rtp_extn_onebytehdr;
    rtp_extn_twobytehdrtype_h  rtp_extn_twobytehdr;

    rtp_extn_element_onebytehdrtype_h                   rtp_extn_elem_onebytehdr_1;
    rtp_extn_element_twobytehdrtype_h                   rtp_extn_elem_twobytehdr_1;
    rtp_extn_element_onebytehdrtype_onebytedata_h       rtp_extn_elem_onebytehdr_1_onebytedata;
    rtp_extn_element_twobytehdrtype_onebytedata_h       rtp_extn_elem_twobytehdr_1_onebytedata;
    rtp_extn_element_onebytehdrtype_threebytedata_h     rtp_extn_elem_onebytehdr_1_threebytedata;
    rtp_extn_element_twobytehdrtype_threebytedata_h     rtp_extn_elem_twobytehdr_1_threebytedata;
    rtp_extn_element_onebytehdrtype_fifteenbytedata_h   rtp_extn_elem_onebytehdr_1_fifteenbytedata;
    rtp_extn_element_twobytehdrtype_twentybytedata_h    rtp_extn_elem_twobytehdr_1_twentybytedata;

    rtp_extn_element_onebytehdrtype_h                   rtp_extn_elem_onebytehdr_2;
    rtp_extn_element_twobytehdrtype_h                   rtp_extn_elem_twobytehdr_2;
    rtp_extn_element_onebytehdrtype_onebytedata_h       rtp_extn_elem_onebytehdr_2_onebytedata;
    rtp_extn_element_twobytehdrtype_onebytedata_h       rtp_extn_elem_twobytehdr_2_onebytedata;
    rtp_extn_element_onebytehdrtype_threebytedata_h     rtp_extn_elem_onebytehdr_2_threebytedata;
    rtp_extn_element_twobytehdrtype_threebytedata_h     rtp_extn_elem_twobytehdr_2_threebytedata;
    rtp_extn_element_onebytehdrtype_fifteenbytedata_h   rtp_extn_elem_onebytehdr_2_fifteenbytedata;
    rtp_extn_element_twobytehdrtype_twentybytedata_h    rtp_extn_elem_twobytehdr_2_twentybytedata;

    rtp_extn_element_onebytehdrtype_h                   rtp_extn_elem_onebytehdr_3;
    rtp_extn_element_twobytehdrtype_h                   rtp_extn_elem_twobytehdr_3;
    rtp_extn_element_onebytehdrtype_onebytedata_h       rtp_extn_elem_onebytehdr_3_onebytedata;
    rtp_extn_element_twobytehdrtype_onebytedata_h       rtp_extn_elem_twobytehdr_3_onebytedata;
    rtp_extn_element_onebytehdrtype_threebytedata_h     rtp_extn_elem_onebytehdr_3_threebytedata;
    rtp_extn_element_twobytehdrtype_threebytedata_h     rtp_extn_elem_twobytehdr_3_threebytedata;
    rtp_extn_element_onebytehdrtype_fifteenbytedata_h   rtp_extn_elem_onebytehdr_3_fifteenbytedata;
    rtp_extn_element_twobytehdrtype_twentybytedata_h    rtp_extn_elem_twobytehdr_3_twentybytedata;

    av1_h   av1;
}