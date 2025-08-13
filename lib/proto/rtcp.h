
#ifndef P4SFU_RTCP_H
#define P4SFU_RTCP_H

#include <arpa/inet.h>
#include <iomanip>

namespace rtcp {

    static const unsigned HDR_LEN = 8;

    enum class pt : std::uint8_t { // https://datatracker.ietf.org/doc/html/rfc5760#section-5
        //! sender report
        sr    = 200,
        //! receiver report
        rr    = 201,
        //! source description
        sdes  = 202,
        //! goodbye
        bye   = 203,
        //! application-defined
        app   = 204,
        //! generic RTP feedback
        rtpfb = 205,
        //! payload-specific feedback
        psfb  = 206
    };

    static std::string pt_name(pt pt) {

        switch (pt) {
            case pt::sr:    return "sr";
            case pt::rr:    return "rr";
            case pt::sdes:  return "sdes";
            case pt::bye:   return "bye";
            case pt::app:   return "app";
            case pt::rtpfb: return "rtpfb";
            case pt::psfb:  return "psfb";
            default:        return "unknown";
        }
    }

    struct hdr {

        // https://datatracker.ietf.org/doc/html/rfc3550#section-6.4.1
        // https://datatracker.ietf.org/doc/html/rfc3550#section-6.4.2

        //! version, padding, reception report count
        std::uint8_t v_p_rc        = 0;
        //! RTCP packet type
        std::uint8_t pt            = 0;
        //! length of the RTCP packet in 32-bit words minus one
        std::uint16_t len          = 0;
        //! SSRC of the sender of this report
        std::uint32_t sender_ssrc  = 0;

        //! version
        [[nodiscard]] unsigned version() const {
            return (v_p_rc >> 6) & 0x03;
        }

        //! padding flag
        [[nodiscard]] unsigned padding() const {
            return (v_p_rc >> 5) & 0x01;
        }

        //! number of reception report blocks present in this message
        [[nodiscard]] unsigned recep_rep_count() const {
            return v_p_rc & 0x1f;
        }

        //! returns the feedback message type (fmt) for RTPFB and PSFB packets
        [[nodiscard]] unsigned fb_fmt() const {
            return v_p_rc & 0x1f;
        }

        //! returns the total length of the RTCP packet in bytes
        //! - does not consider compound packets
        [[nodiscard]] unsigned byte_len() const {
            return ntohs(len) * 4 + 4;
        }

        //! sender report
        struct sr {
            //! NTP timestamp most significant word
            std::uint32_t ntp_ts_msw        = 0;
            //! NTP timestamp least significant word
            std::uint32_t ntp_ts_lsw        = 0;
            //! RTP timestamp
            std::uint32_t rtp_ts            = 0;
            //! sender's packet count
            std::uint32_t sender_pkt_count  = 0;
            //! sender's byte count
            std::uint32_t sender_byte_count = 0;
        };

        //! receiver report
        struct rr {
            //! SSRC this receiver report is about
            std::uint32_t ssrc                = 0;
            //! 8 bits fraction lost, 24 bits cumulative lost
            std::uint32_t frac_lost_cum_lost  = 0;
            //! extended highest sequence number received
            std::uint32_t ext_highest_seq_num = 0;
            //! interarrival jitter
            std::uint32_t jitter              = 0;
            //! last SR
            std::uint32_t lsr                 = 0;
            //! delay since last SR
            std::uint32_t dlsr                = 0;

            //! returns the numerator (0-255) of the fraction of packets lost
            [[nodiscard]] unsigned frac_lost() const {
                return (frac_lost_cum_lost >> 24) & 0xff;
            }

            //! returns the total number of packets lost
            [[nodiscard]] unsigned cum_lost() const {
                return frac_lost_cum_lost & 0x00ffffff;
            }
        };

        //! receiver-estimated bandwidth
        struct remb {
            //! always 0 [draft-alvestrand-rmcat-remb-03]
            std::uint32_t source_ssrc = 0;
            //! constant identifier: "REMB"
            std::uint32_t remb        = 0;
            //! number of SSRCs, mantissa and exponent
            std::uint32_t num_exp_man = 0;
            //! list of SSRCs this estimate relates to
            std::uint32_t ssrcs[1]    = {0};

            //! returns the number of SSRCs this estimate relates to
            [[nodiscard]] unsigned num_ssrcs() const {
                return ntohl(num_exp_man) >> 24;
            }

            //! returns the estimated maximum bandwidth in bits per second
            [[nodiscard]] unsigned bit_rate() const {
                std::uint32_t man = ntohl(num_exp_man) & 0b11'1111'1111'1111'1111;
                std::uint32_t exp = (ntohl(num_exp_man) >> 18) & 0b0011'1111;
                return man << exp;
            }
        };

        //! negative acknowledgement
        struct nack { // https://datatracker.ietf.org/doc/html/rfc4585#section-6.2.1
            //! ssrc of stream with lost packets
            std::uint32_t ssrc = 0;
            //! lost packet id
            std::uint16_t pid = 0;
            //! bitmastk of following lost packets
            std::uint16_t blp = 0;
        };

        //! picture loss indication
        struct pli {
            //! ssrc of stream with lost picture
            std::uint32_t ssrc = 0;
        };

        union {
            //! sender report
            struct sr   sr;
            //! receiver report
            struct rr   rr[1];
            //! receiver-estimated maximum bandwidth
            struct remb remb;
            //! negative acknowledgement
            struct nack nack;
            //! picture loss indication
            struct pli pli;
        } data;
    };

    static std::ostream &operator<<(std::ostream &os, const rtcp::hdr &rtcp) {
        os << "rtcp: v="        << std::dec << rtcp.version()
           << ",p="             << std::dec << rtcp.padding()
           << ",rc="            << std::dec << rtcp.recep_rep_count()
           << ",pt="            << std::dec << (unsigned) rtcp.pt
           << ",len="           << std::dec << ntohs(rtcp.len)
           << ",sender_ssrc=0x" << std::hex << std::setw(8) << std::setfill('0')
           << ntohl(rtcp.sender_ssrc) << std::dec;

        return os;
    }
}

#endif