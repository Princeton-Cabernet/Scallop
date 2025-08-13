
#include "log.h"

Log::Log(Log::level l)
    : _level(l), _nullStream("/dev/null", std::ios::out) {

    if (config.printLabel) {

        switch (l) {
            case Log::level::TRACE:
                operator<<("[TRACE] ");
                break;
            case Log::level::DEBUG:
                operator<<("[DEBUG] ");
                break;
            case Log::level::INFO:
                operator<<("[INFO]  ");
                break;
            case Log::level::WARN:
                operator<<("[WARN]  ");
                break;
            case Log::level::ERROR:
                operator<<("[ERROR] ");
                break;
            default:
                operator<<("[-]     ");
                break;
        }
    }
}

struct Log::config Log::config = {};
Log::level Log::TRACE = Log::level::TRACE;
Log::level Log::DEBUG = Log::level::DEBUG;
Log::level Log::INFO  = Log::level::INFO;
Log::level Log::WARN  = Log::level::WARN;
Log::level Log::ERROR = Log::level::ERROR;
