#ifndef NULLSTRINGSENDER_HPP
#define NULLSTRINGSENDER_HPP

#include "IStringSender.hpp"

namespace fmapp  {
    class NullStringSender : public IStringSender {
    public:
        virtual void send(const char *bytes, size_t length) override {};
        virtual ~NullStringSender() = default;
    };
}

#endif