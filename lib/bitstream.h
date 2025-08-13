
#ifndef P4SFU_BITSTREAM_H
#define P4SFU_BITSTREAM_H

#include <cstdint>
#include <vector>

class Bitstream {

public:
    Bitstream(const unsigned char* buf, std::size_t len);

    [[nodiscard]] unsigned bitCount() const;
    [[nodiscard]] bool operator[](std::size_t idx) const;
    [[nodiscard]] std::uint32_t extract(std::size_t idxFrom, std::size_t n = 1);

private:
    [[nodiscard]] bool _isValidIdx(std::size_t idx);
    std::vector<bool> _data;
};

#endif
