#pragma once

#include <cmath>

namespace transport {
namespace geo {

struct Coordinates {
    double lat;
    double lng;

    bool operator==(const Coordinates& other) const
    {
        return lat == other.lat && lng == other.lng;
    }

    bool operator!=(const Coordinates& other) const
    {
        return !(*this == other);
    }
};

inline double ComputeDistance(Coordinates from, Coordinates to)
{
    using namespace std;

    if (from == to) {
        return 0.0;
    }

    constexpr double kEarthRadiusInMeters = 6371000.0;
    constexpr double kDegToRad = 3.1415926535 / 180.0;

    auto DegToRad = [](double degrees) {
        return degrees * kDegToRad;
    };

    const double phi1 = DegToRad(from.lat);
    const double lambda1 = DegToRad(from.lng);
    const double phi2 = DegToRad(to.lat);
    const double lambda2 = DegToRad(to.lng);

    const double delta_phi = phi2 - phi1;
    const double delta_lambda = lambda2 - lambda1;

    const double a = pow(sin(delta_phi / 2), 2) +
        cos(phi1) * cos(phi2) *
        pow(sin(delta_lambda / 2), 2);

    const double c = 2 * asin(sqrt(a));

    return kEarthRadiusInMeters * c;
}

} // namespace geo
} // namespace transport