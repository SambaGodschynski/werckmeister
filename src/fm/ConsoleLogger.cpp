#include "ConsoleLogger.h"
#include <iostream>

namespace fm {
    void ConsoleLogger::babble(const WriteToStreamF& f)
    {
        if (_logLevel > LevelBabble) {
            return;
        }
        std::cout << "[info] ";
        writeMessage(f);
    }
    void ConsoleLogger::warn(const WriteToStreamF& f)
    {
        if (_logLevel > LevelWarn) {
            return;
        }
        std::cout << "[warning] ";
        writeMessage(f);
    }
    void ConsoleLogger::error(const WriteToStreamF& f)
    {
        if (_logLevel > LevelError) {
            return;
        }
        std::cout << "[error] ";
        writeMessage(f);
    }
    void ConsoleLogger::writeMessage(const WriteToStreamF &f)
    {
        f(std::cout);
        std::cout << std::endl;
    }
}