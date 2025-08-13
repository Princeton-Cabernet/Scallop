
#ifndef STD_IN_H
#define STD_IN_H

#include <boost/asio.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>
#include <optional>

using namespace boost;
class StdIn {

public:

    typedef std::function<void (StdIn&, char*, std::size_t)> OnInputHandler;

    explicit StdIn(asio::io_context& io)
            : _io(io),
              _stream(_io, STDIN_FILENO) {

        _read();
    }

    void onInput(OnInputHandler&& f) {
        _onInput = std::move(f);
    }

private:

    void _read() {

        _stream.async_read_some(asio::buffer(_readBuf, _READ_BUF_LEN),
            [this](system::error_code ec, std::size_t len) {

            if (!ec) {

                if (_onInput) {
                    (*_onInput)(*this, _readBuf, len);
                }

                _read();
            }
        });
    }

    asio::io_context& _io;
    boost::asio::posix::stream_descriptor _stream;
    static const std::size_t _READ_BUF_LEN = 1024;
    char _readBuf[_READ_BUF_LEN] = {};
    std::optional<OnInputHandler> _onInput = std::nullopt;
};

#endif
