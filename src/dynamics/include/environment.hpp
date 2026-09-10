#pragma once

#include <Eigen/Dense>

class Environment
{
public:
    static const Environment& instance() {
        return Environment::_instance;
    }

    Eigen::Vector3d getGravity([[maybe_unused]] const Eigen::Vector3d& position) const {
        return Eigen::Vector3d(0.0, 0.0, 9.805);
    }

    double getAirDensity([[maybe_unused]] const Eigen::Vector3d& position) const {
        return 1.225;
    }

private:
    Environment() = default;
    static Environment _instance;
};

inline Environment Environment::_instance;