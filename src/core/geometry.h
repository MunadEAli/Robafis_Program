#pragma once
// Pure geometry checks for the robot envelope. No Arduino code, so it runs in PC tests too.

#include <math.h>

namespace geometry {

// Radius swept by the robot's farthest corner when it spins on the spot
// around the middle of the wheel axle.
inline float turnRadius(float width, float frontLen, float rearLen) {
    const float l  = frontLen > rearLen ? frontLen : rearLen;
    const float hw = width * 0.5f;
    return sqrtf(hw * hw + l * l);
}

inline bool widthFits(float width, float maxWidth) {
    return width <= maxWidth;
}

inline bool turnClears(float width, float frontLen, float rearLen, float maxRadius) {
    return turnRadius(width, frontLen, rearLen) <= maxRadius;
}

// Longest the robot may reach in front of (or behind) the axle for a given width.
inline float maxHalfLength(float width, float maxRadius) {
    const float hw = width * 0.5f;
    const float d  = maxRadius * maxRadius - hw * hw;
    return d > 0.0f ? sqrtf(d) : 0.0f;
}

}  // namespace geometry
