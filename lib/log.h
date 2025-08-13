
#ifndef LOG_H
#define LOG_H

#include <iostream>
#include <fstream>

class Log {

public:

    enum class level {
        ERROR  = 0,
        WARN   = 1,
        INFO   = 2,
        DEBUG  = 3,
        TRACE  = 4
    };

    struct config {
        Log::level level = level::WARN;
        bool printLabel = false;
    };

    static struct config config;

    Log() = delete;
    explicit Log(Log::level l);

    template<typename T>
    std::ostream& operator<<(const T& msg) {

        if(_level <= config.level) {
            return (_level == Log::level::ERROR ? std::cerr : std::cout) << msg;
        }

        return _nullStream;
    }

    static level DEBUG;
    static level INFO;
    static level WARN;
    static level ERROR;
    static level TRACE;

private:
    Log::level _level;
    std::fstream _nullStream;
};

#endif
