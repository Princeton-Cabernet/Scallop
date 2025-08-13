
#include "bitstream.h"

#include <stdexcept>

Bitstream::Bitstream(const unsigned char* buf, std::size_t len)
        : _data(len * 8) {

    for (auto byteIndex = 0; byteIndex < len; byteIndex++) {
        for (auto bitIndex = 7; bitIndex >= 0; bitIndex--) {
            _data[(byteIndex * 8 + (8 - bitIndex)) - 1] = (buf[byteIndex] >> bitIndex) & 1;
        }
    }
}

bool Bitstream::operator[](std::size_t idx) const {

    if (idx >= _data.size()) {
        throw std::invalid_argument("Bitstream: operator[]: index out of range");
    }

    return _data[idx];
}

unsigned Bitstream::bitCount() const {
    return _data.size();
}

std::uint32_t Bitstream::extract(std::size_t idxFrom, std::size_t n /* = 1 */) {

    std::uint32_t res = 0;

    if (n > 0 && n <= 32 && _isValidIdx(idxFrom) && _isValidIdx(idxFrom + n - 1)) {

        for (auto i = 0; i < n; i++) {
            res |= ((std::uint32_t) _data[idxFrom + i]) << (n - i - 1);
        }

        return res;

    } else {
        throw std::invalid_argument("Bitstream: extract: index out of range");
    }
}

bool Bitstream::_isValidIdx(std::size_t idx) {
    return idx < _data.size();
}
