#pragma once

#include <Eigen/Dense>

class Ahrs {
public:
    virtual ~Ahrs() = default;

    virtual Eigen::Vector3d getPositionNed() const = 0;
    virtual Eigen::Vector3d getVelocityNed() const = 0;
    virtual Eigen::Quaterniond getOrientation() const = 0;

    virtual Eigen::Vector3d getAcceleration() const = 0;
    virtual Eigen::Vector3d getAngularVelocity() const = 0;

    virtual Eigen::Vector3d getAirspeed() const = 0;

    virtual Eigen::Vector3d getLla() const = 0;
};
