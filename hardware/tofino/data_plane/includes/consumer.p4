// Control block to determine which host should consume the packet
// (1) The host/server that is serving the videoconferencing clients
// (2) The controller
// (3) The data plane

control Packet_Consumer(
    in header_t hdr,
    in PortId_t ingress_port,
    inout packet_type_t packet_type
) {
    action set_packet_reflection_consumer(packet_type_t consumer_type) {
        packet_type = consumer_type;
    }

    table packet_reflector {
        key = {
            ingress_port : exact;
            hdr.udp.isValid() : exact;
            hdr.udp.dst_port : exact;
        }
        actions = {
            set_packet_reflection_consumer;
        }
        const entries = {
            // Reflect packets coming from Bluefield3 SFU on UDP port BF3_LOOPBACK_UDP_PORT back to the Bluefield3 SFU
            (BLUEFIELD3_PORT_P0, true, BF3_LOOPBACK_UDP_PORT) : set_packet_reflection_consumer(PACKET_TYPE_CONSUME_BLUEFIELD_P0);
            (BLUEFIELD3_PORT_P1, true, BF3_LOOPBACK_UDP_PORT) : set_packet_reflection_consumer(PACKET_TYPE_CONSUME_BLUEFIELD_P1);
        }
        size = 4;
        default_action = set_packet_reflection_consumer(NULL);
    }

    action set_packet_consumer(packet_type_t consumer_type) {
        packet_type = consumer_type;
    }

    table packet_consumer {
        key = {
            ingress_port : exact;
            hdr.ethernet.ether_type : ternary;
            hdr.arp.isValid() : exact;
            hdr.arp.dst_ip : ternary;
            hdr.arp.op_code : ternary;
            hdr.ipv4.isValid() : exact;
            hdr.ipv4.src_addr : ternary;
            hdr.ipv4.dst_addr : ternary;
        }
        actions = {
            set_packet_consumer;
        }
        const entries = {
            // Forward packets destined to Bluefield3 SFU to the Bluefield DPU
            (CABERNET802_PORT, _, false, _, _, true, CABERNET802_IP, BF3_SFU_IPV4_ADDR) : set_packet_consumer(PACKET_TYPE_CONSUME_BLUEFIELD_P0);
            (CABMEATY02_PORT,  _, false, _, _, true, CABMEATY02_IP,  BF3_SFU_IPV4_ADDR) : set_packet_consumer(PACKET_TYPE_CONSUME_BLUEFIELD_P0);
            // Forward packets coming from Bluefield3 SFU to the clients host or the controller
            (BLUEFIELD3_PORT_P0, _, false, _, _, true, BF3_SFU_IPV4_ADDR, CABMEATY02_IP) : set_packet_consumer(PACKET_TYPE_CONSUME_CLIENTS_HOST);
            (BLUEFIELD3_PORT_P0, _, false, _, _, true, BF3_SFU_IPV4_ADDR, CABERNET802_IP) : set_packet_consumer(PACKET_TYPE_CONSUME_CONTROLLER);
            (BLUEFIELD3_PORT_P1, _, false, _, _, true, BF3_SFU_IPV4_ADDR, CABMEATY02_IP) : set_packet_consumer(PACKET_TYPE_CONSUME_CLIENTS_HOST);
            (BLUEFIELD3_PORT_P1, _, false, _, _, true, BF3_SFU_IPV4_ADDR, CABERNET802_IP) : set_packet_consumer(PACKET_TYPE_CONSUME_CONTROLLER);
            // Consume ARP requests locally at the SFU
            (CABERNET802_PORT, ETHERTYPE_ARP, true, SFU_IPV4_ADDR, ARP_REQUEST, false, _, _) : set_packet_consumer(PACKET_TYPE_CONSUME_SFU);
            (CABMEATY02_PORT, ETHERTYPE_ARP, true, SFU_IPV4_ADDR, ARP_REQUEST, false, _, _) : set_packet_consumer(PACKET_TYPE_CONSUME_SFU);
            // Forward ARP packets to the clients host if coming from controller and vice-versa
            (CABERNET802_PORT, ETHERTYPE_ARP, true, _, _, false, _, _) : set_packet_consumer(PACKET_TYPE_CONSUME_CLIENTS_HOST);
            (CABMEATY02_PORT, ETHERTYPE_ARP, true, _, _, false, _, _) : set_packet_consumer(PACKET_TYPE_CONSUME_CONTROLLER);
            // Consume IP packets locally at the SFU
            (CABERNET802_PORT, _, false, _, _, true, CABERNET802_IP, SFU_IPV4_ADDR) : set_packet_consumer(PACKET_TYPE_CONSUME_SFU);
            (CABMEATY02_PORT, _, false, _, _, true, CABMEATY02_IP, SFU_IPV4_ADDR) : set_packet_consumer(PACKET_TYPE_CONSUME_SFU);
            // Forward IP packets to the clients host if coming from the controller IP and vice-versa
            (CABERNET802_PORT, _, false, _, _, true, CABERNET802_IP, CABMEATY02_IP) : set_packet_consumer(PACKET_TYPE_CONSUME_CLIENTS_HOST);
            (CABMEATY02_PORT, _, false, _, _, true, CABMEATY02_IP, CABERNET802_IP) : set_packet_consumer(PACKET_TYPE_CONSUME_CONTROLLER);
            // Forward packets coming from the switch agent to the controller or the clients host
            (CPU_PORT, ETHERTYPE_ARP, true, _, _, false, _, _) : set_packet_consumer(NULL);
            (CPU_PORT, _, false, _, _, true, SFU_IPV4_ADDR, CABMEATY02_IP) : set_packet_consumer(PACKET_TYPE_CONSUME_CLIENTS_HOST);
            (CPU_PORT, _, false, _, _, true, SFU_IPV4_ADDR, CABERNET802_IP) : set_packet_consumer(PACKET_TYPE_CONSUME_CONTROLLER);
            // Forward IP packets to the clients host if coming from the controller port and vice-versa
            (CABERNET802_PORT, _, false, _, _, true, _, _) : set_packet_consumer(PACKET_TYPE_CONSUME_CLIENTS_HOST);
            (CABMEATY02_PORT, _, false, _, _, true, _, _) : set_packet_consumer(PACKET_TYPE_CONSUME_CONTROLLER);
        }
        size = 32;
        default_action = set_packet_consumer(NULL);
    }

    apply {
        packet_reflector.apply(); // For Bluefield setup
        if (packet_type == NULL) {
            packet_consumer.apply();
        }
    }
}