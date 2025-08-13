
#ifndef NET_IPV4_H
#define NET_IPV4_H

#include <cstdint>
#include <functional>
#include <regex>
#include <sstream>
#include <iomanip>
#include <stdexcept>

namespace net {

    namespace eth {

        const unsigned ADDR_LEN = 6;
        const unsigned HDR_LEN = 14;

        enum class type : std::uint16_t {
            ipv4 = 0x0800
        };

        struct addr {
            std::uint8_t bytes[ADDR_LEN] = {0};

            [[nodiscard]] std::string to_str() const {
                std::stringstream ss;
                ss << std::hex << std::setw(2) << std::setfill('0') << (unsigned) bytes[0] << ":"
                   << std::hex << std::setw(2) << std::setfill('0') << (unsigned) bytes[1] << ":"
                   << std::hex << std::setw(2) << std::setfill('0') << (unsigned) bytes[2] << ":"
                   << std::hex << std::setw(2) << std::setfill('0') << (unsigned) bytes[3] << ":"
                   << std::hex << std::setw(2) << std::setfill('0') << (unsigned) bytes[4] << ":"
                   << std::hex << std::setw(2) << std::setfill('0') << (unsigned) bytes[5];
                return ss.str();
            }
        };

        struct hdr {
            addr dst_addr;
            addr src_addr;
            std::uint16_t ether_type = 0;

            void init_default() {
                std::fill(dst_addr.bytes, dst_addr.bytes + ADDR_LEN, 0);
                std::fill(src_addr.bytes, src_addr.bytes + ADDR_LEN, 0);
                ether_type = 0;
            }
        };
    }

    namespace ipv4 {

        const unsigned HDR_LEN = 20;

        enum class proto : std::uint8_t {
            icmp = 1,
            tcp = 6,
            udp = 17
        };

        struct hdr { // 20
            std::uint8_t version_ihl = 0; // 1
            std::uint8_t type_of_service = 0; // 1
            std::uint16_t total_length = 0; // 2
            std::uint16_t id = 0; // 2
            std::uint16_t fragment_offset = 0; // 2 // 8
            std::uint8_t time_to_live = 0; // 1
            std::uint8_t next_proto_id = 0; // 1
            std::uint16_t hdr_checksum = 0; // 2
            std::uint32_t src_addr = 0; // 4 // 8
            std::uint32_t dst_addr = 0; // 4

            [[nodiscard]] unsigned ihl_bytes() const {
                return (version_ihl & 0x0f) * 4;
            }

            void init_default() {
                version_ihl     = 0x45;
                type_of_service = 0;
                total_length    = 0;
                id              = 0;
                fragment_offset = 0;
                time_to_live    = 64;
                next_proto_id   = 0;
                hdr_checksum    = 0;
                src_addr        = 0;
                dst_addr        = 0;
            }
        };
    }

    namespace udp {

        const unsigned HDR_LEN = 8;

        struct hdr { // 8
            std::uint16_t src_port    = 0; // 2
            std::uint16_t dst_port    = 0; // 2
            std::uint16_t dgram_len   = 0; // 2
            std::uint16_t dgram_cksum = 0; // 2 // 8
        };
    }

    class IPv4 {

    public:

        IPv4() = default;

        explicit IPv4(std::uint32_t addr) : _addr(addr) { }

        explicit IPv4(const std::string& s) {

            int stoiResult = 0;

            for (std::size_t i = 0, next_dot = 0, pos = 0; i < 4; ++i, pos = next_dot + 1) {
                next_dot = s.find('.', pos);

                if (next_dot == std::string::npos && i < 3) {
                    throw (std::invalid_argument("net::IPv4: invalid address: " + s));
                }

                if (next_dot != std::string::npos && i == 3) {
                    throw (std::invalid_argument("net::IPv4: invalid address: " + s));
                }

                try {
                    stoiResult = std::stoi(s.substr(pos, next_dot - pos));
                } catch (...) {
                    throw (std::invalid_argument("net::IPv4: invalid address: " + s));
                }

                if (stoiResult >= 0 && stoiResult <= 255) {
                    _addr |= ((std::uint8_t) stoiResult) << (8 * (3 - i));
                } else {
                    throw (std::invalid_argument("net::IPv4: invalid address: " + s));
                }
            }
        }

        [[nodiscard]] std::uint32_t num() const {
            return _addr;
        }

        [[nodiscard]] bool operator==(const IPv4& other) const {
            return _addr == other._addr;
        }

        [[nodiscard]] bool operator!=(const IPv4& other) const {
            return !(*this == other);
        }

        [[nodiscard]] std::string str() const {

            std::stringstream ss;

            ss << ((_addr >> 24) & 0xff) << "." << ((_addr >> 16) & 0xff) << "."
               << ((_addr >> 8) & 0xff) << "." << ((_addr >> 0) & 0xff);

            return ss.str();
        }

        friend std::ostream &operator<<(std::ostream &os, IPv4 const& ip) {
            return os << ip.str();
        }

        [[nodiscard]] bool isInSubnet(const IPv4& subnet, std::uint8_t prefixLength) const {

            std::uint32_t mask = (prefixLength == 0) ? 0 : (~0U << (32 - prefixLength));
            return (_addr & mask) == (subnet._addr & mask);
        }

    private:
        std::uint32_t _addr = 0;
    };

    //! An IP v4 address and a port number
    class IPv4Port {
    public:

        IPv4Port() = default;

        explicit IPv4Port(const net::IPv4& ip, std::uint16_t port)
            : _ip(ip), _port(port) { }

        explicit IPv4Port(const std::string& ip, std::uint16_t port)
            : _ip{ip}, _port{port} { }

        explicit IPv4Port(std::uint32_t ip, std::uint16_t port)
            : _ip{ip}, _port{port} { }

        IPv4Port(const IPv4Port&) = default;

        [[nodiscard]] inline IPv4 ip() const {
            return _ip;
        }

        [[nodiscard]] inline std::uint16_t port() const {
            return _port;
        }

        bool operator==(const IPv4Port& other) const {
            return _ip == other._ip && _port == other._port;
        }

        bool operator!=(const IPv4Port& other) const {
            return !(*this == other);
        }

        friend std::ostream &operator<<(std::ostream &os, IPv4Port const& ipPort) {
            return os << ipPort._ip << ":" << ipPort._port;
        }

    private:
        IPv4 _ip = net::IPv4{};
        std::uint16_t _port = 0;
    };

    //! An IP v4 network address plus a prefix length
    class IPv4Prefix {

    public:
        explicit IPv4Prefix(const net::IPv4& ip, std::uint8_t prefix)
            : _ip{ip}, _prefix{prefix} { }

        explicit IPv4Prefix(const std::string& ip, std::uint8_t prefix)
            : _ip{ip}, _prefix{prefix} { }

        explicit IPv4Prefix(std::uint32_t ip, std::uint8_t prefix)
            : _ip{ip}, _prefix{prefix} { }

        IPv4Prefix(const IPv4Prefix&) = default;

        [[nodiscard]] inline IPv4 ip() const {
            return _ip;
        }

        [[nodiscard]] inline std::uint8_t prefix() const {
            return _prefix;
        }

        bool operator==(const IPv4Prefix& other) const {
            return _ip == other._ip && _prefix == other._prefix;
        }

        bool operator!=(const IPv4Prefix& other) const {
            return !(*this == other);
        }

        friend std::ostream &operator<<(std::ostream &os, IPv4Prefix const& ipPrefix) {
            return os << ipPrefix._ip << "/" << ipPrefix._prefix;
        }

    private:
        IPv4 _ip             = {};
        std::uint8_t _prefix = 0;
    };
}

namespace std {
    template<> struct hash<net::IPv4Port> {
        std::size_t operator()(const net::IPv4Port& d) const noexcept  {
            std::size_t h = 0;
            h |= (std::size_t) d.ip().num() << 16u;
            h |= (std::size_t) d.port()     << 0u;
            return h;
        }
    };
}

#endif
