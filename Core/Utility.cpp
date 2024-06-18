#include "Utility.h"

namespace utils {
    // Utility function to normalize an angle to the range [0, 2π)
    double NormalizeAngle(double angle) {
        const double TWO_PI = 2.0f * Pi;
        while (angle >= TWO_PI) angle -= TWO_PI;
        while (angle < 0.0f) angle += TWO_PI;
        return angle;
    }

    // Function to lerp between two angles
    double LerpAngle(double startAngle, double endAngle, float t) {
        const double TWO_PI = 2.0f * Pi;

        startAngle = NormalizeAngle(startAngle);
        endAngle = NormalizeAngle(endAngle);

        double difference = endAngle - startAngle;

        if (difference > Pi) {
            difference -= TWO_PI;
        }
        else if (difference < -Pi) {
            difference += TWO_PI;
        }

        double result = startAngle + t * difference;
        return NormalizeAngle(result);
    }
}
