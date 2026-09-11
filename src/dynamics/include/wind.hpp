#pragma once

#include <Eigen/Dense>

#include <algorithm>
#include <cmath>
#include <random>

struct WindModelParameters {
    Eigen::Vector3d mean_wind{0.0, 0.0, 0.0};
    Eigen::Vector3d turbulence_sigma{2.0, 2.0, 1.0};
    Eigen::Vector3d turbulence_tau{8.0, 8.0, 4.0};
    double reference_height{10.0};
    double height_exponent{0.14};
};

class WindModel {
public:
    WindModel(WindModelParameters params = WindModelParameters{}) : _params(params) {}

    void update(double dt, double altitude) {
        for (int i = 0; i < 3; ++i) {
            const double tau = std::max(_params.turbulence_tau[i], 1e-3);
            const double a = std::exp(-dt / tau);
            const double b = _params.turbulence_sigma[i] * std::sqrt(1.0 - a * a);
            _turbulence[i] = a * _turbulence[i] + b * _normal(_rng);
        }

        altitude = std::max(altitude, 0.1);
        const double scale = std::pow(altitude / _params.reference_height, _params.height_exponent);
        _wind = _params.mean_wind * scale + _turbulence;
    }

    Eigen::Vector3d get() const {
        return _wind;
    }

    void reset() { _turbulence.setZero(); }

private:
    WindModelParameters _params;
    Eigen::Vector3d _turbulence{0.0, 0.0, 0.0};
    std::mt19937 _rng{std::random_device{}()};
    std::normal_distribution<double> _normal{0.0, 1.0};

    Eigen::Vector3d _wind{0.0, 0.0, 0.0};
};