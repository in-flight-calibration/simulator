#pragma once

#include <cfloat>
#include <random>

#include "sensors/sensor.hpp"

#include <aircraft_msgs/msg/sensor_ins.hpp>


struct SensorInsParams
{
    Eigen::Quaterniond alignment = Eigen::Quaterniond::Identity();
    double tau = 0.0;
    double noise_sd = 0.0;
};

class SensorIns : public Sensor<aircraft_msgs::msg::SensorIns, SensorInsParams>
{
public:
    using Sensor<aircraft_msgs::msg::SensorIns, SensorInsParams>::Sensor;

private:
    Eigen::Quaterniond _last_q = Eigen::Quaterniond::Identity();

    std::mt19937 _gen{std::random_device{}()};
    std::normal_distribution<double> _noise_dist{0.0, 1.0};


    aircraft_msgs::msg::SensorIns process(
        double dt, 
        const aircraft_msgs::msg::Groundtruth& groundtruth, 
        [[maybe_unused]] const aircraft_msgs::msg::Environment& environment
    ) override {

        aircraft_msgs::msg::SensorIns message;
        message.header.stamp = _parent.now();

        const Eigen::Quaterniond q = Eigen::Quaterniond(
            groundtruth.orientation.w,
            groundtruth.orientation.x,
            groundtruth.orientation.y,
            groundtruth.orientation.z
        );

        const Eigen::Quaterniond q_est = calculateQuaternionEstimation(dt, q);

        message.orientation.w = q_est.w();
        message.orientation.x = q_est.x();
        message.orientation.y = q_est.y();
        message.orientation.z = q_est.z();

        return message;
    }

    Eigen::Quaterniond calculateQuaternionEstimation(double dt, const Eigen::Quaterniond& q) {
        const double alpha = 1.0 - std::exp(-dt / _params.tau);
        const Eigen::Quaterniond dq = (_last_q.conjugate() * q).normalized();
        const Eigen::AngleAxisd aa(dq);
        const Eigen::AngleAxisd aa_lagged(alpha * aa.angle(), aa.axis());
        const Eigen::Quaterniond dq_lagged(aa_lagged);
        const Eigen::Quaterniond q_lagged = (_last_q * dq_lagged).normalized();
        const Eigen::Quaterniond q_error = getNoiseError();
        _last_q = q_lagged;
        return (q_error * _params.alignment * q_lagged).normalized();
    }

    Eigen::Quaterniond getNoiseError() {
        Eigen::Vector3d noisy_error = Eigen::Vector3d(
            _noise_dist(_gen),
            _noise_dist(_gen),
            _noise_dist(_gen)
        ) * _params.noise_sd;

        const double error_norm = noisy_error.norm();
        if (error_norm < DBL_EPSILON) {
            return Eigen::Quaterniond::Identity();
        }

        return Eigen::Quaterniond(
            Eigen::AngleAxisd(error_norm, noisy_error / error_norm)
        );
    }
};