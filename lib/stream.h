
#ifndef P4_SFU_STREAM_H
#define P4_SFU_STREAM_H

#include "net/net.h"
#include "p4sfu.h"
#include "proto/sdp.h"
#include "sfu_config.h"
#include <iostream>
#include <variant>

namespace p4sfu {

    //! Wrapper around a media stream as defined by an SDP media description
    class Stream {

        friend class Participant;

    public:

        struct DataPlaneConfig {
            DataPlaneConfig() { }

            //! Address and port the stream uses to send or receive RTP and RTCP traffic
            net::IPv4Port addr;
            //! ICE username fragment
            std::string iceUfrag;
            //! ICE password
            std::string icePwd;
            //! Media type of the stream
            MediaType mediaType = MediaType::audio;
            //! SSRC used for the main media stream
            SSRC mainSSRC = 0;
            //! SSRC used for retransmissions
            SSRC rtxSSRC = 0;
        };

        //! DataPlaneConfig specialization for send streams
        struct SendStreamConfig : public DataPlaneConfig { };

        //! DataPlaneConfig specialization for receive streams
        struct ReceiveStreamConfig : public DataPlaneConfig {
            //! SSRC the receiver uses to send RTCP feedback
            SSRC rtcpSSRC = 0;
            SSRC rtcpRtxSSRC = 0;
        };

        Stream() = delete;
        Stream(const Stream&) = default;
        Stream& operator=(const Stream&) = default;

        //! Initializes the stream using a SDP media description
        //!  - requires the SSRC description of the corresponding send stream if the stream is
        //!    a receive stream
        explicit Stream(const SDP::MediaDescription& media,
                        const p4sfu::SFUConfig& sfuConfig,
                        const SDP::SSRCDescription& ssrcDescription = {});

        //! Returns the original offer as an SDP media description.
        [[nodiscard]] const SDP::MediaDescription& description() const;

        //! Returns an offer or answer media description to be forwarded to other meeting
        //! participants containing the respective media stream, incorporating the SFU as the
        //! sending peer.
        [[nodiscard]] SDP::MediaDescription offer(const p4sfu::SFUConfig& sfuConfig) const;

        [[nodiscard]] const std::variant<SendStreamConfig, ReceiveStreamConfig>&
            dataPlaneConfig() const;

        template<typename ConfigType>
        inline ConfigType dataPlaneConfig() const {
            return std::get<ConfigType>(_dataPlaneConfig);
        }

        [[nodiscard]] std::string formattedSummary() const;

    private:
        SDP::MediaDescription _description; // offer or answer

        std::variant<SendStreamConfig, ReceiveStreamConfig> _dataPlaneConfig;
    };
}

#endif
