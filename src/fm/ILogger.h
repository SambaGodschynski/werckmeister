#ifndef ILOGGER_H
#define ILOGGER_H

#include <ostream>
#include <functional>

#define WRMLogLambda(x) [&](auto &log) { (x); } 

namespace fm {
    class ILogger {
    public:
        enum LogLevel { LevelBabble, LevelWarn, LevelError, LevelNone };
        typedef std::function<void(std::ostream&)> WriteToStreamF;
        virtual void babble(const WriteToStreamF&) = 0;
        virtual void warn(const WriteToStreamF&) = 0;
        virtual void error(const WriteToStreamF&) = 0;
        virtual void logLevel(LogLevel lvl) = 0;
        virtual LogLevel logLevel() const = 0;
    };
}

#endif