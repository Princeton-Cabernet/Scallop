
#ifndef TIMER_H
#define TIMER_H

#include <boost/asio.hpp>
#include <optional>

using namespace boost;

class Timer {

public:
    explicit Timer(asio::io_context& io, unsigned intervalMs)
        : _intervalMs(intervalMs),
          _timer(io) {

        _run();
    }

    void onTimer(std::function<void (Timer&)>&& f) {
        _onTimer = std::move(f);
    }

    inline unsigned long count() const {
        return _count;
    }

private:

    void _run() {
        _timer.async_wait([this](std::error_code ec) {

            if (!ec) {

                if (_onTimer) {
                    (*_onTimer)(*this);
                }

                _count++;
                _timer.expires_after(std::chrono::milliseconds(_intervalMs));
                _run();
            }
        });
    }

    asio::steady_timer _timer;
    unsigned _intervalMs;
    std::optional<std::function<void (Timer&)>> _onTimer = std::nullopt;
    unsigned long _count = 0;
};

#endif
