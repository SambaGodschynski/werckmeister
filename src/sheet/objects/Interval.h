#ifndef INTERVAL_H
#define INTERVAL_H

#include <fm/units.hpp>
#include <climits>
#include "ASheetObject.h"

namespace sheet {
    struct Interval : public ASheetObject {
		enum { INVALID_VALUE = INT_MAX };
        int value;
		bool operator <(const Interval &b) const { return value < b.value; }
		bool valid() const { return value != INVALID_VALUE; }
    };
}

#endif