
#ifndef FILE_DESCRIPTOR_H
#define FILE_DESCRIPTOR_H

#include <boost/asio.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>
#include <optional>

using namespace boost;

class FileDescriptor {

public:

    typedef std::function<void (FileDescriptor&)> OnReadReadyHandler;

    explicit FileDescriptor(asio::io_context& io, int fd)
            : _io{io}, _fd{fd}, _stream{io, fd} {

        _read();
    }

    void onReadReady(OnReadReadyHandler&& f) {
        _onReadReady = std::move(f);
    }

    [[nodiscard]] int fd() const {
        return _fd;
    }

private:

    void _read() {
        _stream.async_wait(asio::posix::stream_descriptor::wait_read,
            [this](const system::error_code& ec) -> void {

               if (!ec) {
                   if (_onReadReady) {
                       (*_onReadReady)(*this);
                   }
                   _read();
               }
        });
    }

    asio::io_context& _io;
    int _fd;
    boost::asio::posix::stream_descriptor _stream;
    std::optional<OnReadReadyHandler> _onReadReady = std::nullopt;
};

#endif
