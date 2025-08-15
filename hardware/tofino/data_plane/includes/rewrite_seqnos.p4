// Control block to rewrite sequence numbers for media packets

control Rewrite_Sequence_Numbers(
    inout header_t hdr,
    in packet_type_t packet_type
) {

    RcvStreamSignature_t rcv_stream_signature;
    Hash<RcvStreamSignature_t>(HashAlgorithm_t.CRC32,
            CRCPolynomial<RcvStreamSignature_t>(CRC_POLY_3,false,false,false,0,0)) hash_rcv_stream_signature;
    action compute_rcv_stream_signature() {
        rcv_stream_signature = hash_rcv_stream_signature.get({
            SEED_RCV_STREAM_SIGNATURE,
            hdr.ipv4.src_addr,
            hdr.ipv4.dst_addr,
            hdr.udp.src_port,
            hdr.udp.dst_port,
            hdr.rtp.ssrc
        });
    }

    RcvStreamDataWidth_t rcv_stream_index;
    Hash<RcvStreamDataWidth_t>(HashAlgorithm_t.CRC16,
            CRCPolynomial<RcvStreamDataWidth_t>(CRC_POLY_0,false,false,false,0,0)) hash_rcv_stream_index;
    action compute_rcv_stream_index() {
        rcv_stream_index = (RcvStreamDataWidth_t)hash_rcv_stream_index.get({
            SEED_RCV_STREAM_INDEX,
            rcv_stream_signature
        });
        // Test case to test collision in the FT
        // rtt_ingress.rangetracker_index = 0x1;
    }

    // Drop count per receive stream: registers
    Register<RcvStreamSignature_t, RcvStreamDataWidth_t>(REGSIZE_RCV_STREAM) reg_rcv_stream_signature;
    Register<Counter16_t, RcvStreamDataWidth_t>(REGSIZE_RCV_STREAM) reg_drop_count;

    // Check receive stream signature match
    RegisterAction<RcvStreamSignature_t, RcvStreamDataWidth_t, bit<8>>(reg_rcv_stream_signature) set_or_match_rcv_stream_signatures = {
        void apply(inout RcvStreamSignature_t mem_cell, out bit<8> match_type) {
            if (mem_cell == 0) {
                mem_cell = rcv_stream_signature;
                match_type = 0;
            } else if (mem_cell == rcv_stream_signature) {
                match_type = 1;
            } else {
                match_type = 2;
            }
        } };
    bit<8> rcv_stream_signature_match_type;
    action act_set_or_match_rcv_stream_signature() {
        rcv_stream_signature_match_type = set_or_match_rcv_stream_signatures.execute(rcv_stream_index);
    }

    // Increment receive stream drop count
    RegisterAction<Counter16_t, RcvStreamDataWidth_t, Counter16_t>(reg_drop_count) increment_drop_count = {
        void apply(inout Counter16_t mem_cell) {
            mem_cell = mem_cell + 1;
        } };
    action act_increment_rcv_stream_drop_count() {
        increment_drop_count.execute(rcv_stream_index);
    }

    // Read receive stream drop count
    RegisterAction<Counter16_t, RcvStreamDataWidth_t, Counter16_t>(reg_drop_count) read_drop_count = {
        void apply(inout Counter16_t mem_cell, out Counter16_t drop_count) {
            drop_count = mem_cell;
        } };
    Counter16_t rcv_stream_drop_count;
    action act_read_rcv_stream_drop_count() {
        rcv_stream_drop_count = read_drop_count.execute(rcv_stream_index);
    }

    apply {
        compute_rcv_stream_signature();
        compute_rcv_stream_index();
        act_set_or_match_rcv_stream_signature();

        if (rcv_stream_signature_match_type == 1 && packet_type == AV1_LAYER_DROP) {
            act_increment_rcv_stream_drop_count();
        }
        else if (rcv_stream_signature_match_type == 1) {
            act_read_rcv_stream_drop_count();
        }

        // Rewrite sequence number
        if (rcv_stream_signature_match_type == 1 && packet_type != AV1_LAYER_DROP) {
            hdr.rtp.sequence_num = hdr.rtp.sequence_num - rcv_stream_drop_count;
        }
    }
}