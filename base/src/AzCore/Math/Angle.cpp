/*
    File: Angle.cpp
    Author: Philip Haynes
*/

#include "../math.hpp"

namespace AzCore {

#ifdef AZCORE_MATH_F32
Radians32 angleDiff(Angle32 from, Angle32 to) {
    Radians32 diff = Radians32(to) - Radians32(from);
    while (diff >= pi) {
        diff -= tau;
    }
    while (diff < -pi) {
        diff += tau;
    }
    return diff;
}
#endif
#ifdef AZCORE_MATH_F64
Radians64 angleDiff(Angle64 from, Angle64 to) {
    Radians64 diff = Radians64(to) - Radians64(from);
    while (diff >= pi64) {
        diff -= tau64;
    }
    while (diff < -pi64) {
        diff += tau64;
    }
    return diff;
}
#endif

} // namespace AzCore
