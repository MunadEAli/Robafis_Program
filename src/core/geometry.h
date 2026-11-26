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

// Largest turning radius that clears the deposit lines when the robot spins in the middle
// of a gap between two deposits: g = colony line -> deposit line, gap = space between deposits.
inline float maxTurnRadiusInGap(float g, float gapWidth) {
    const float h = gapWidth * 0.5f;
    return sqrtf(g * g + h * h);
}

// How far the grip point must reach beyond the robot's side to sit over a target
// (e.g. a cube centre) that is targetDist away from the colony line the robot follows.
inline float requiredArmReach(float targetDist, float width) {
    return targetDist - width * 0.5f;
}

// How far the grip point gets beyond the robot's side at full extension.
inline float armReachBeyondSide(float gripRetractedY, float stroke, float width) {
    return gripRetractedY + stroke - width * 0.5f;
}

// The robot centre (axle middle) must stay inside [fenceMin, fenceMax] along the line
// so the turning circle plus a margin never crosses the territory boundary.
inline float fenceMin(float turnR, float margin) { return turnR + margin; }
inline float fenceMax(float trackW, float turnR, float margin) { return trackW - turnR - margin; }

// Where the axle must stop so the arm (armX ahead of the axle) is level with targetX.
// facingPlusX: robot faces +x (scanning / picking); false after the 180-degree turn (placing).
inline float axleXForTarget(float targetX, float armX, bool facingPlusX) {
    return facingPlusX ? targetX - armX : targetX + armX;
}

inline bool inRange(float v, float lo, float hi) { return v >= lo && v <= hi; }

// True if the point (x, y) in the robot frame lies inside the outer body.
inline bool insideBody(float x, float y, float width, float frontLen, float rearLen) {
    const float hw = width * 0.5f;
    return inRange(x, -rearLen, frontLen) && inRange(y, -hw, hw);
}

}  // namespace geometry
