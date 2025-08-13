
#ifndef P4SFU_UTIL_H
#define P4SFU_UTIL_H

#include <fstream>
#include <iomanip>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>

namespace util {

    static void replaceAll(std::string& haystack, const std::string& needle,
                           const std::string& replace) {

        for (size_t start = 0; (start = haystack.find(needle, start)) != std::string::npos;) {
            haystack.replace(start, needle.length(), replace);
            start += replace.length();
        }
    }

    static std::uint32_t crc32(std::uint32_t crc, const unsigned char *buf, size_t len) {

        int k;
        crc = ~crc;
        while (len--) {
            crc ^= *buf++;
            for (k = 0; k < 8; k++)
                crc = crc & 1 ? (crc >> 1) ^ 0xedb88320 : crc >> 1;
        }


        return ~crc;
    }

    static std::tuple<std::string, unsigned short> parseIPPort(const std::string& s) {

        const std::regex r(R"(([0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3})\:([0-9]{1,5}))");
        std::smatch m;

        if (std::regex_match(s, m, r) && m.size() == 3) {
            return std::make_tuple<std::string, unsigned short>(m[1].str(), std::stoul(m[2].str()));
        } else {
            throw std::invalid_argument(
                    "Could not parse addr.: " + s + " is not of format IPv4:PORT");
        }
    }

    static std::tuple<std::string, unsigned char> parseCIDR(const std::string& s) {

        std::regex cidrRegex(R"(^(\d+\.\d+\.\d+\.\d+)/([0-9]{1,2})$)");
        std::smatch match;

        if (std::regex_match(s, match, cidrRegex)) {
            return std::make_tuple<std::string, unsigned char>(match[1].str(),
                (unsigned char) std::stoi(match[2].str()));
        } else {
            throw std::invalid_argument(
                    "Could not parse cidr: " + s + " is not in format IPv4/MASK");
        }
    }

    template<typename T>
    static std::string hexString(const T buf, std::size_t len, unsigned space = 0) {

        std::stringstream ss;

        for (auto i = 0; i < len; i++) {

            if (space && i > 0 && i % space == 0)
                ss << " ";

            ss << std::hex << std::setw(2) << std::setfill('0') << (unsigned) buf[i];
        }
        return ss.str();
    }


    static std::vector<std::string> splitString(const std::string& str, const std::string& delim) {

        std::vector<std::string> parts;
        std::size_t last = 0, next = 0;

        while ((next = str.find(delim, last)) != std::string::npos) {
            parts.push_back(str.substr(last, next-last));
            last = next + delim.size();
        }

        parts.push_back(str.substr(last));
        return parts;
    }

    template<typename... Args>
    static std::string formatString(const std::string& format, Args ...args) {

        int sizeS = std::snprintf(nullptr, 0, format.c_str(), args...) + 1;

        if (sizeS <= 0)
            throw std::runtime_error("util::formatString: formatting error");

        auto size = static_cast<size_t>(sizeS);

        std::unique_ptr<char[]> buf( new char[ size ]);
        std::snprintf(buf.get(), size, format.c_str(), args...);

        return {buf.get(), buf.get() + size - 1};
    }

    template <typename T>
    static std::string joinStrings(const std::vector<T>& elements, const std::string& join = " ") {

        std::stringstream ss;

        for (auto i = 0; i < elements.size(); i++) {

            ss << elements[i];

            if (i < elements.size() - 1)
                ss << join;
        }

        return ss.str();
    }

    static std::string bufferString(const char* buf, unsigned len) {

        std::stringstream ss;
        ss.write(buf, (long)(len-(buf[len-1] == '\n' ? 1 : 0)));
        return ss.str();
    }

    static std::uint32_t extractBits(const std::vector<bool>& bits, unsigned from, unsigned len = 1) {

        if (from + len > bits.size())
            throw std::invalid_argument("util:extractBits: requested range outside of bit vector");

        if (len > 32 || len < 1)
            throw std::invalid_argument("util:extractBits: len must be > 0 and <= 32");

        std::uint32_t a = 0;

        for (auto i = from, j = len - 1; i < from + len; i++, j--)
            a |= (bits[i] << j);

        return a;
    }

    static std::pair<std::uint32_t, unsigned> extractNonSymmetric(const std::vector<bool>& bits,
                                                                  unsigned from, unsigned n) {

        unsigned w = 0, x = n;

        while (x != 0) {
            x = x >> 1;
            w++;
        }

        unsigned m = (1 << w) - n;

        std::uint32_t v = extractBits(bits, from, w - 1);

        if (v < m) {
            return std::make_pair(v, w-1);
        }

        unsigned extra_bit = extractBits(bits, from + w - 1, 1);

        return std::make_pair((v << 1) - m + extra_bit, w);
    }

    static std::string sigName(int sig) {

        switch (sig) {
            case 1:  return "SIGHUP";
            case 2:  return "SIGINT";
            case 3:  return "SIGQUIT";
            case 8:  return "SIGFPE";
            case 9:  return "SIGKILL";
            case 14: return "SIGALRM";
            case 15: return "SIGTERM";
            default: return "OTHER";
        }
    }

    static std::string readFile(const std::string& fileName) {

        std::ifstream file{fileName};

        if (!file.is_open())
            throw std::runtime_error{"util::readFile: failed to open " + fileName};

        return std::string{std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}};
    }

    static std::string escapeJSONForHTML(const std::string& json) {

        std::string result;

        for (char c: json) {
            switch (c) {
                case '&':
                    result.append("&amp;");
                    break;
                case '<':
                    result.append("&lt;");
                    break;
                case '>':
                    result.append("&gt;");
                    break;
                case '"':
                    result.append("&quot;");
                    break;
                case '\'':
                    result.append("&#39;");
                    break;
                default:
                    result.push_back(c);
            }
        }
        return result;
    }
}

#endif
