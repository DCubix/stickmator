#include "Utility.h"

#include <algorithm>

namespace utils {
    // Utility function to normalize an angle to the range [0, 2π)
    double NormalizeAngle(double angle) {
        const double TWO_PI = 2.0f * Pi;
        while (angle >= TWO_PI) angle -= TWO_PI;
        while (angle < 0.0f) angle += TWO_PI;
        return angle;
    }

    static double Repeat(float t, float maxVal) {
        return std::clamp(t - std::floorf(t / maxVal) * maxVal, 0.0f, maxVal);
    }

    // Function to lerp between two angles
    double LerpAngle(double startAngle, double endAngle, float t) {
        double delta = Repeat(endAngle - startAngle, 2.0f * Pi);
        if (delta > Pi) delta -= 2.0f * Pi;
        return startAngle + delta * std::clamp(t, 0.0f, 1.0f);
    }
}
