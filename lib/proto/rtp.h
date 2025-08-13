#ifndef P4_SFU_RTP_H
#define P4_SFU_RTP_H

#include <arpa/inet.h>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <optional>
#include <ostream>
#include <sstream>

namespace rtp {

    static const unsigned HDR_LEN = 12;
    static const unsigned EXT_PROFILE_HDR_LEN = 4;

    enum class ext_profile : std::uint16_t {
        one_byte = 0xbede,
        two_byte = 0x1000,
        other    = 0
    };

    static bool contains_rtp_or_rtcp(const unsigned char* buf, std::size_t len) {

        return len >= 12 && ((buf[0] >> 6) & 0x03) == 0x02;
    }

    //! returns the extension identifier
    //! @param ext_buf pointer to the first byte of an extension
    //! @param profile extension profile type to use (1-byte or 2-byte)
    inline static unsigned short ext_type(const unsigned char* ext_buf,
        const ext_profile profile = ext_profile::one_byte) {

        if (profile != ext_profile::one_byte && profile != ext_profile::two_byte) {
            throw std::domain_error("rtp::ext_type: unsupported extension profile");
        }

        if (profile == ext_profile::one_byte) {
            return (*ext_buf >> 4) & 0x0f;
        } else if (profile == ext_profile::two_byte) {
            return ext_buf[0];
        }

        return 0;
    }

    //! returns the length of the extension in bytes excluding the 1-byte header
    //! @param ext_buf pointer to the first byte of an extension
    //! @param profile extension profile type to use (1-byte or 2-byte)
    inline static unsigned short ext_len(const unsigned char* ext_buf,
        const ext_profile profile = ext_profile::one_byte) {

        if (profile != ext_profile::one_byte && profile != ext_profile::two_byte) {
            throw std::domain_error("rtp::ext_len: unsupported extension profile");
        }

        if (profile == ext_profile::one_byte) {
            return (*ext_buf & 0x0f) + 1;
        } else if (profile == ext_profile::two_byte) {
            return (unsigned short)(ext_buf[1]);
        }

        return 0;
    }

    struct extension_profile_hdr {
        //! profile-specific identifier
        std::uint16_t profile;
        //! length of the extension in 32-bit words, excluding the 32 bits of the extension header
        std::uint16_t len;

        //! returns the length of the extensions in bytes
        [[nodiscard]] std::size_t byte_len() const {
            return ntohs(len) * 4;
        }
    };

    //! wrapper around an RTP one-byte header extension for simplified management and serialization
    struct ext {

        //! extension identifier
        unsigned short type = 0;

        //! length of the extension in bytes excluding the 1-byte header
        unsigned short len = 0;

        //! extension data
        unsigned char data[128] = {0};

        bool operator==(const struct ext& other) const {
            return type == other.type
                   && len  == other.len
                   && std::strncmp((const char*) data, (const char*) other.data, len) == 0;
        }

        bool operator!=(const struct ext& other) const {
            return !(*this == other);
        }

        //! constructs an extension struct from a pointer to the beginning of an extension
        static ext from_buf(const unsigned char* buf, auto profile = ext_profile::one_byte) {

            unsigned ext_hdr_len = 0;

            if (profile == ext_profile::one_byte) {
                ext_hdr_len = 1;
            } else if (profile == ext_profile::two_byte) {
                ext_hdr_len = 2;
            }

            ext e;
            e.type = ext_type(buf, profile);
            e.len = ext_len(buf, profile);
            std::memcpy(e.data, buf + ext_hdr_len, e.len);
            return e;
        }
    };

    struct one_byte_extension_hdr {

        std::uint8_t _id_len;

        [[nodiscard]] unsigned short id() const {
            return (_id_len >> 4) & 0x0f;
        }

        [[nodiscard]] unsigned short len() const {
            return _id_len & 0x0f;
        }
    };

    struct two_byte_extension_hdr {

        std::uint8_t _id;
        std::uint8_t _len;

        [[nodiscard]] unsigned short id() const {
            return _id;
        }

        [[nodiscard]] unsigned short len() const {
            return _len;
        }
    };


    //! RTP header as specified in https://datatracker.ietf.org/doc/html/rfc3550#section-5.1
    struct hdr {

        std::uint8_t  v_p_x_cc = 0;
        std::uint8_t  m_pt     = 0;
        std::uint16_t seq      = 0;
        std::uint32_t ts       = 0;
        std::uint32_t ssrc     = 0;

        [[nodiscard]] unsigned version() const {
            return (v_p_x_cc >> 6) & 0x03;
        }

        [[nodiscard]] unsigned padding() const {
            return (v_p_x_cc >> 5) & 0x01;
        }

        [[nodiscard]] unsigned extension() const {
            return (v_p_x_cc >> 4) & 0x01;
        }

        [[nodiscard]] unsigned csrc_count() const {
            return v_p_x_cc & 0x0f;
        }

        [[nodiscard]] unsigned marker() const {
            return m_pt >> 7 & 0x01;
        }

        [[nodiscard]] unsigned payload_type() const {
            return m_pt & 0x7f;
        }

        //! returns the RTP extension profile
        //! @note returns ext_profile::other if no or non-standard extensions present
        [[nodiscard]] ext_profile extension_profile() const {

            const auto* ext_hdr = this->extension_profile_header();

            if (ntohs(ext_hdr->profile) == static_cast<std::uint16_t>(ext_profile::one_byte)) {
                return ext_profile::one_byte;
            } else if (ntohs(ext_hdr->profile) == static_cast<std::uint16_t>(ext_profile::two_byte)) {
                return ext_profile::two_byte;
            } else {
                return ext_profile::other;
            }
        }

        //! returns a pointer to the RTP extension profile header or nullpointer if there
        //! are no extensions
        [[nodiscard]] extension_profile_hdr* extension_profile_header() const {
            return extension() ? (extension_profile_hdr*)(((const unsigned char*)this) + 12) : nullptr;
        }

        //! returns a vector of pointers to the first byte of each extension
        //! @note use ext_type() and ext_len() to access the extension identifier and length.
        //! @note use pointer arithmetic to determine offset from an anchor (e.g., RTP header
        //!       begin).
        [[nodiscard]] std::vector<const unsigned char*> extension_ptrs() const {

            std::vector<const unsigned char*> exts;

            // do not support csrc's for now
            if (this->csrc_count() || !this->extension()) {
                return exts;
            }

            const auto* ext_buf = reinterpret_cast<const unsigned char*>(this) + 12;

            if (this->extension_profile() == ext_profile::other) {
                return exts;
            }

            unsigned ext_hdr_len = 0;

            if (this->extension_profile() == ext_profile::one_byte) {
                ext_hdr_len = 1;
            } else if (this->extension_profile() == ext_profile::two_byte) {
                ext_hdr_len = 2;
            }

            std::uint16_t total_ext_bytes = ((ext_buf[2] << 8) + (ext_buf[3])) * 4;

            for (unsigned i = 4; i < total_ext_bytes + 4;) {
                if (ext_buf[i] != 0) { // next extension
                    exts.push_back(ext_buf + i);
                    i += (ext_len(ext_buf + i, this->extension_profile()) + ext_hdr_len);
                } else { // padding
                    i++;
                }
            }

            return exts;
        }

        //! returns a pointer to the first byte of the extension with the given identifier
        //! @note use ext_type() and ext_len() to access the extension identifier and length.
        //! @note use pointer arithmetic to determine offset from an anchor (e.g., RTP header
        //!       begin).
        //! @return pointer to first byte of extension or nullptr if the extension is not found
        [[nodiscard]] const unsigned char* extension_ptr(unsigned type) const {

            for (const auto* ext: this->extension_ptrs()) {
                if (ext_type(ext, this->extension_profile()) == type) {
                    return ext;
                }
            }

            return nullptr;
        }

        //! returns a vector of extension structs
        [[nodiscard]] std::vector<ext> extensions() const {

            std::vector<ext> extensions;

            for (const auto* ext: this->extension_ptrs()) {
                extensions.emplace_back(ext::from_buf(ext, this->extension_profile()));
            }

            return extensions;
        }

        //! returns an extension struct with the given identifier or std::nullopt if not found
        [[nodiscard]] std::optional<ext> extension(unsigned short id) const {

            for (const auto* extPtr: this->extension_ptrs()) {
                if (ext_type(extPtr, this->extension_profile()) == id) {
                    auto ext = ext::from_buf(extPtr, this->extension_profile());
                    return std::optional<struct rtp::ext>{ext};
                }
            }

            return std::nullopt;
        }

        //! returns whether this packet is a RTCP packet (payload type 200-207)
        [[nodiscard]] bool is_rtcp() const {
            return (m_pt >= 200 && m_pt <= 207);
        }
    };

    /*
    //! DEPRECATED
    static std::optional<struct ext> getExt(const rtp::hdr* hdr, unsigned type) {

        struct ext ext;

        const auto* buf = reinterpret_cast<const unsigned char*>(hdr);
        const auto* extBuf = buf + 12;

        if (hdr->csrc_count() == 0 && hdr->extension() == 1) {

            std::uint16_t extBytes = ((extBuf[2] << 8) + (extBuf[3])) * 4;

            for (unsigned i = 4; i < extBytes + 4;) {

                if (extBuf[i] != 0) {

                    unsigned short extType = (extBuf[i] >> 4) & 0x0f;
                    unsigned short extLen = (extBuf[i] & 0x0f) + 1;

                    if (extType == type) {
                        ext.type = extType;
                        ext.len = extLen;
                        std::memcpy(ext.data, extBuf + i + 1,ext.len);
                        return ext;
                    }

                    i += (extLen + 1);

                } else { // padding
                    i++;
                }
            }
        }

        return std::nullopt;
    }
    */

    //! serializes the RTP extensions to a buffer and returns the number of bytes written
    static std::size_t serialize_extensions(const std::vector<ext>& extensions,
                                            unsigned char* buf, std::size_t len,
                                            ext_profile profile) {

        if (profile != ext_profile::one_byte) {
            throw std::domain_error("rtp::serialize_extensions: only 1-byte exts supported");
        }

        //TODO: return 0 buffer lenght is insufficient

        std::size_t bytes_written = 4;
        buf[0] = 0xbe, buf[1] = 0xde;

        for (auto i = 0; i < extensions.size() && bytes_written < len-1; i++) {
            buf[bytes_written++] = (((extensions[i].type & 0x0f) << 4) & 0xf0)
                          | (((extensions[i].len - 1) & 0x0f) & 0x0f);

            for (auto j = 0; j < extensions[i].len && bytes_written < len-1; j++)
                buf[bytes_written++] = extensions[i].data[j];
        }

        // pad with zeros to make the total length a multiple of 4 if required
        for (auto i = 0; i < bytes_written % 4 && bytes_written < len-1; i++)
            buf[bytes_written++] = 0;

        *((unsigned short*)(buf + 2)) = htons(((bytes_written - 4) / 4));

        return bytes_written;
    }

    static std::ostream& operator<<(std::ostream& os, const rtp::hdr& rtp) {
        os << "rtp: v=" << std::dec << rtp.version()
           << ",p=" << std::dec << rtp.padding()
           << ",x=" << std::dec << rtp.extension()
           << ",cc=" << std::dec << rtp.csrc_count()
           << ",m=" << std::dec << rtp.marker()
           << ",pt=" << std::dec << rtp.payload_type()
           << ",seq=" << std::dec << ntohs(rtp.seq)
           << ",ts=" << std::dec << ntohl(rtp.ts)
           << ",ssrc=0x" << std::hex << std::setw(8) << std::setfill('0')
           << ntohl(rtp.ssrc) << std::dec;

        return os;
    }
}

#endif
