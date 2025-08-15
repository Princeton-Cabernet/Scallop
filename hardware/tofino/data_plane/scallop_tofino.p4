#include <core.p4>
#if __TARGET_TOFINO__ == 1
#include <tna.p4>
#elif __TARGET_TOFINO__ == 2
#include <t2na.p4>
#endif

// 0: Model, 1: Testbed (ASIC)
#define RUN_MODE 1

#include "includes/definitions.p4"
#if RUN_MODE == 0
#include "includes/ports_model.p4"
#elif RUN_MODE == 1
#include "includes/ports_testbed.p4"
#endif

#include "includes/types.p4"
#include "includes/sfu_properties.p4"
#include "includes/table_sizes.p4"
#include "includes/packet_types.p4"

#include "includes/headers.p4"
#include "includes/metadata.p4"
#include "includes/parsers.p4"
#include "includes/deparsers.p4"

#include "includes/consumer.p4"
#include "includes/rewrite_seqnos.p4"

control SwitchIngress(
        inout header_t hdr,
        inout ig_metadata_t ig_md,
        in ingress_intrinsic_metadata_t ig_intr_md,
        in ingress_intrinsic_metadata_from_parser_t ig_prsr_md,
        inout ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md,
        inout ingress_intrinsic_metadata_for_tm_t ig_tm_md) {

    Packet_Consumer() packet_consumer;

    action no_op() { }

    action drop() {
        ig_dprsr_md.drop_ctl = 0x1;
    }

    action copy_to_cpu() {
        ig_tm_md.copy_to_cpu = 1;
    }

    action bypass_egress() {
        ig_tm_md.bypass_egress = 1;
        ig_md.bypass_egress = 1;
    }

    action craft_arp_reply() {
        hdr.arp.op_code = ARP_REPLY;
        hdr.arp.dst_mac = hdr.arp.src_mac;
        hdr.arp.src_mac = SFU_MAC_ADDR;
        hdr.arp.dst_ip  = hdr.arp.src_ip;
        hdr.arp.src_ip  = SFU_IPV4_ADDR;
        hdr.ethernet.dst_addr = hdr.ethernet.src_addr;
        hdr.ethernet.src_addr = SFU_MAC_ADDR;
    }

    action craft_icmp_response() {
        hdr.icmp.type = 0; // ICMP Echo Reply
        hdr.ipv4.dst_addr = hdr.ipv4.src_addr;
        hdr.ipv4.src_addr = SFU_IPV4_ADDR;
        hdr.ethernet.dst_addr = hdr.ethernet.src_addr;
        hdr.ethernet.src_addr = SFU_MAC_ADDR;
    }

    action set_mac_for_cpu_incoming() {
        hdr.ethernet.dst_addr = CABMEATY02_MAC;
        hdr.ethernet.src_addr = SFU_MAC_ADDR;
    }

    action setup_replication (MulticastGroupId_t mgid, ReplicationId__t packet_rid, L2ExclusionId__t l2_xid) {
        ig_tm_md.mcast_grp_a = mgid;
        ig_tm_md.rid = packet_rid;
        ig_tm_md.level2_exclusion_id = l2_xid;
        // Set the destination port to an invalid value
        ig_tm_md.ucast_egress_port = (PortId_t)0x1ff;
    }

    table packet_replication {
        key = {
            hdr.ipv4.src_addr : exact;
            hdr.udp.src_port : exact;
            ig_md.rtp_rtcp_ssrc : exact;
        }
        actions = {
            setup_replication;
            no_op;
        }
        size = 256;
        const default_action = no_op;
    }

    action setup_forwarding (ipv4_addr_t ip_dst_addr, tp_port_t udp_dst_port) {
        hdr.ipv4.src_addr = SFU_IPV4_ADDR;
        hdr.ipv4.dst_addr = ip_dst_addr;
        hdr.udp.src_port = SFU_UDP_PORT;
        hdr.udp.dst_port = udp_dst_port;
        hdr.ethernet.src_addr = SFU_MAC_ADDR;
        hdr.ethernet.dst_addr = CABMEATY02_MAC;
    }

    table recv_report_forwarding {
        key = {
            hdr.ipv4.src_addr : exact;
            hdr.udp.src_port : exact;
        }
        actions = {
            setup_forwarding;
            drop;
        }
        size = 256;
        const default_action = drop;
    }

    table nack_pli_forwarding {
        key = {
            hdr.ipv4.src_addr : exact;
            hdr.udp.src_port : exact;
        }
        actions = {
            setup_forwarding;
            drop;
        }
        size = 256;
        const default_action = drop;
    }

    action set_av1_template_id_mod(packet_type_t mod) {
        ig_md.template_id_mod = mod;
    }

    table av1_template_id_mod_lookup {
        key = {
            hdr.av1.dep_template_id : exact;
        }
        actions = {
            set_av1_template_id_mod;
        }
        size = 64;
        const default_action = set_av1_template_id_mod(64);
    }

    action set_level1_exclusion_id (L1ExclusionId__t l1_xid) {
        ig_tm_md.level1_exclusion_id = l1_xid;
    }

    table video_layer_suppression {
        key = {
            hdr.ipv4.src_addr     : exact;
            hdr.udp.src_port      : exact;
            hdr.rtp.ssrc          : exact;
            ig_md.template_id_mod : exact;
        }
        actions = {
            set_level1_exclusion_id;
            no_op;
        }
        size = 256;
        const default_action = no_op;
    }

    action set_egress_port(PortId_t egress_port) {
        ig_tm_md.ucast_egress_port = egress_port;
    }

    apply {

        // 1. Determine base packet type (ARP/ICMP/UDP/TCP)
        // 2. Determine response (ARP reply, ICMP echo reply, send AV1 to CPU, etc.)
        // 3. Replication action
        // 4. Forwarding decision

        ig_md.packet_type = NULL;
        ig_md.bypass_egress = 0;

        packet_consumer.apply(hdr, ig_intr_md.ingress_port, ig_md.packet_type);

        // Packet invalid; drop it
        if (ig_md.packet_type == NULL) {
            drop();
            bypass_egress();
        }
        // Forward packet to the Bluefield3 SFU port P0
        else if (ig_md.packet_type == PACKET_TYPE_CONSUME_BLUEFIELD_P0) {
            ig_tm_md.ucast_egress_port = BLUEFIELD3_PORT_P0;
            bypass_egress();
            ig_md.packet_type = PACKET_TYPE_CONSUME_FORWARDED;
        }
        // Forward packet to the Bluefield3 SFU port P1
        else if (ig_md.packet_type == PACKET_TYPE_CONSUME_BLUEFIELD_P1) {
            ig_tm_md.ucast_egress_port = BLUEFIELD3_PORT_P1;
            bypass_egress();
            ig_md.packet_type = PACKET_TYPE_CONSUME_FORWARDED;
        }
        // Forward packet to the controller
        else if (ig_md.packet_type == PACKET_TYPE_CONSUME_CLIENTS_HOST) {
            ig_tm_md.ucast_egress_port = CABMEATY02_PORT;
            hdr.ethernet.src_addr = CABERNET802_MAC;
            hdr.ethernet.dst_addr = CABMEATY02_MAC;
            bypass_egress();
            ig_md.packet_type = PACKET_TYPE_CONSUME_FORWARDED;
        }
        // Forward packet to the clients host
        else if (ig_md.packet_type == PACKET_TYPE_CONSUME_CONTROLLER) {
            ig_tm_md.ucast_egress_port = CABERNET802_PORT;
            hdr.ethernet.src_addr = CABMEATY02_MAC;
            hdr.ethernet.dst_addr = CABERNET802_MAC;
            bypass_egress();
            ig_md.packet_type = PACKET_TYPE_CONSUME_FORWARDED;
        }
        // Packet is an ARP request destined for this SFU
        else if (hdr.arp.isValid() && hdr.arp.op_code == ARP_REQUEST) {
            if (hdr.arp.dst_ip == SFU_IPV4_ADDR) {
                ig_md.packet_type = PACKET_TYPE_SFU_ARP;
            }
        }
        // Packet is an ICMP request destined for this SFU
        else if (hdr.ipv4.isValid() && hdr.icmp.isValid()) {
            ig_md.packet_type = PACKET_TYPE_SFU_ICMP;
        }
        // Packet has arrived from the switch CPU and needs to be forwarded
        else if (ig_intr_md.ingress_port == CPU_PORT) {
            ig_md.packet_type = PACKET_TYPE_CPU_INCOMING;
        }
        // Packet is non-ARP IPv4 and is destined for this SFU
        else if (hdr.ipv4.isValid() && hdr.ipv4.dst_addr == SFU_IPV4_ADDR) {
            // Packet is UDP and is destined for this SFU port
            if (hdr.udp.isValid() && hdr.udp.dst_port == SFU_UDP_PORT) {
                ig_md.packet_type = PACKET_TYPE_SFU_UDP;
            }
            // Packet is TCP
            else if (hdr.tcp.isValid()) {
                ig_md.packet_type = PACKET_TYPE_SFU_TCP;
            }
        }

        // Craft an ARP reply
        if (ig_md.packet_type == PACKET_TYPE_SFU_ARP) {
            craft_arp_reply();
            set_egress_port(ig_intr_md.ingress_port);
            bypass_egress();
        }
        // Craft an ICMP response
        else if (ig_md.packet_type == PACKET_TYPE_SFU_ICMP) {
            craft_icmp_response();
            set_egress_port(ig_intr_md.ingress_port);
            bypass_egress();
        }
        // Forward packets arriving from the CPU port
        else if (ig_md.packet_type == PACKET_TYPE_CPU_INCOMING) {
            set_mac_for_cpu_incoming();
            set_egress_port(CABMEATY02_PORT);
            bypass_egress();
        }
        // Drop non-ARP and non-UDP packets, and all packets not destined for this SFU
        else if (ig_md.packet_type != PACKET_TYPE_CONSUME_FORWARDED && ig_md.packet_type != PACKET_TYPE_SFU_UDP) {
            drop();
            bypass_egress();
        }
        // STUN packet from clients host
        else if (ig_md.packet_type != PACKET_TYPE_CONSUME_FORWARDED && hdr.stun.isValid()) {
            copy_to_cpu();
            bypass_egress();
        }
        // Copy AV1 media packets associated with key frames to CPU
        else if (ig_md.packet_type != PACKET_TYPE_CONSUME_FORWARDED && hdr.av1.isValid()) {
            av1_template_id_mod_lookup.apply();
            if (ig_md.template_id_mod == 0) {
                copy_to_cpu();
            }
        }
        
        // Set up media packet replication to all receivers
        if (ig_md.packet_type != PACKET_TYPE_CONSUME_FORWARDED && hdr.rtp.isValid()) {
            ig_md.rtp_rtcp_ssrc = hdr.rtp.ssrc;
            ig_md.packet_type = PACKET_TYPE_APP_REPLICATE;
        }
        // Set up sender report replication to all receivers
        else if (ig_md.packet_type != PACKET_TYPE_CONSUME_FORWARDED && hdr.rtcp.packet_type == RTCP_SR) {
            ig_md.rtp_rtcp_ssrc = hdr.rtcp.sender_ssrc;
            ig_md.packet_type = PACKET_TYPE_APP_REPLICATE;
        }
        // Forward RR to sender selectively
        else if (ig_md.packet_type != PACKET_TYPE_CONSUME_FORWARDED && hdr.rtcp.packet_type == RTCP_RR) {
            recv_report_forwarding.apply();
            ig_md.packet_type = PACKET_TYPE_APP_FORWARD;
            bypass_egress();
        }
        // Forward NACK/PLI to sender
        else if (ig_md.packet_type != PACKET_TYPE_CONSUME_FORWARDED && (hdr.rtcp.packet_type == RTCP_NACK || hdr.rtcp.packet_type == RTCP_PLI)) {
            nack_pli_forwarding.apply();
            ig_md.packet_type = PACKET_TYPE_APP_FORWARD;
            bypass_egress();
        }

        // Packet replication
        if (ig_md.packet_type == PACKET_TYPE_APP_REPLICATE) {
            packet_replication.apply();
        }

        // Perform rate adaptation on video packets
        if (ig_md.packet_type != PACKET_TYPE_CONSUME_FORWARDED && hdr.av1.isValid()) {
            video_layer_suppression.apply();
        }

        if (ig_md.packet_type != PACKET_TYPE_CONSUME_FORWARDED) {
            ig_tm_md.rid = 2;
            ig_tm_md.level1_exclusion_id = 1;
            ig_tm_md.level2_exclusion_id = 1;
        }

        if (ig_md.bypass_egress == 0) {
            ig_md.bridged_md.template_id_mod = ig_md.template_id_mod;
            ig_md.bridged_md.setValid();
        }
    }
}

control SwitchEgress(
        inout header_t hdr,
        inout eg_metadata_t eg_md,
        in egress_intrinsic_metadata_t eg_intr_md,
        in egress_intrinsic_metadata_from_parser_t eg_intr_md_from_prsr,
        inout egress_intrinsic_metadata_for_deparser_t eg_intr_dprs_md,
        inout egress_intrinsic_metadata_for_output_port_t eg_intr_oport_md) {

    Rewrite_Sequence_Numbers() rewrite_seqnos;
    
    action no_op() { }
    
    action set_destination_headers (ipv4_addr_t ip_dst_addr, tp_port_t udp_dst_port) {
        hdr.ethernet.dst_addr = CABMEATY02_MAC;
        hdr.ipv4.dst_addr = ip_dst_addr;
        hdr.udp.dst_port  = udp_dst_port;
        hdr.udp.checksum = 0x0;
    }

    table ipv4_route {
        key = {
            hdr.ipv4.src_addr     : exact;
            hdr.udp.src_port      : exact;
            eg_md.rtp_rtcp_ssrc   : exact;
            eg_intr_md.egress_rid : exact; // RID is no longer the only identifier for the receiver and not a bottleneck therefore
        }
        actions = {
            set_destination_headers;
            no_op;
        }
        size = 256;
        default_action = no_op;
    }

    action set_source_to_sfu () {
        hdr.ethernet.src_addr = SFU_MAC_ADDR;
        hdr.ipv4.src_addr = SFU_IPV4_ADDR;
        hdr.udp.src_port  = SFU_UDP_PORT;
    }

    apply {
        if (eg_md.bridged_md.isValid()) {
            eg_md.template_id_mod = eg_md.bridged_md.template_id_mod;
            eg_md.bridged_md.setInvalid();
        }

        if (eg_intr_md.egress_rid == 0 && eg_intr_md.egress_port == CPU_PORT) {
            eg_md.packet_type = PACKET_TYPE_CONSUME_CPU;
        }

        if (hdr.rtp.isValid()) {
            eg_md.rtp_rtcp_ssrc = hdr.rtp.ssrc;
            if (eg_md.packet_type != PACKET_TYPE_CONSUME_CPU) {
                ipv4_route.apply();
            }
        }
        else if (hdr.rtcp.isValid()) {
            eg_md.rtp_rtcp_ssrc = hdr.rtcp.sender_ssrc;
            if (eg_md.packet_type != PACKET_TYPE_CONSUME_CPU) {
                ipv4_route.apply();
            }
        }
        else {
            eg_intr_dprs_md.drop_ctl = 0x1;
        }

        if ((hdr.rtp.isValid() || hdr.rtcp.isValid()) && eg_md.packet_type != PACKET_TYPE_CONSUME_CPU) {
            set_source_to_sfu();
        }

        if (eg_md.template_id_mod != 64) {
            rewrite_seqnos.apply(hdr, eg_md.packet_type);
        }
    }
}

Pipeline(SwitchIngressParser(),
         SwitchIngress(),
         SwitchIngressDeparser(),
         SwitchEgressParser(),
         SwitchEgress(),
         SwitchEgressDeparser()) pipe;

Switch(pipe) main;