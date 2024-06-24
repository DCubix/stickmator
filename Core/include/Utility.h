#pragma once

#define _USE_MATH_DEFINES
#include <cmath>

constexpr double Pi = 3.14159265358979323846f;
constexpr double hPi = Pi / 2.0f;
constexpr int FrameRate = 15;

namespace utils {
    // Utility function to normalize an angle to the range [0, 2π)
    double NormalizeAngle(double angle);

    // Function to lerp between two angles
    double LerpAngle(double startAngle, double endAngle, float t);
}
