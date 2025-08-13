
#ifndef P4SFU_P4SFU_H
#define P4SFU_P4SFU_H

#include <cstdint>

namespace p4sfu {

    enum class MediaType {
        audio = 0,
        video = 1
    };

    typedef std::uint32_t SSRC;
    typedef std::uint16_t SwitchPort;

    static std::string mediaTypeString(MediaType type) {
        switch (type) {
            case MediaType::audio: return "audio";
            case MediaType::video: return "video";
            default:               return "";
        }
    }
}

#endif
