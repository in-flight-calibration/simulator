#pragma once

#include <random>
#include <numbers>

#include "sensors/sensor.hpp"

#include <aircraft_msgs/msg/sensor_gnss.hpp>

struct SensorGnssParams
{
    double horizonal_position_noise_sd = 0.0;
    double vertical_position_noise_sd = 0.0;

    double velocity_noise_sd = 0.0;
};

class SensorGnss : public Sensor<aircraft_msgs::msg::SensorGnss, SensorGnssParams>
{
public:
    using Sensor<aircraft_msgs::msg::SensorGnss, SensorGnssParams>::Sensor;

private:
    std::mt19937 _gen{std::random_device{}()};
    std::normal_distribution<double> _horizonal_position_dist{0.0, 1.0};
    std::normal_distribution<double> _vertical_position_dist{0.0, 1.0};
    std::normal_distribution<double> _velocity_dist{0.0, 1.0};

    aircraft_msgs::msg::SensorGnss process(
        [[maybe_unused]] double dt, 
        const aircraft_msgs::msg::Groundtruth& groundtruth, 
        [[maybe_unused]] const aircraft_msgs::msg::Environment& environment
    ) override {
        aircraft_msgs::msg::SensorGnss message;
        message.header.stamp = _parent.now();
        updateHorizontalPosition(groundtruth, message);
        updateVerticalPosition(groundtruth, message);
        updateVelocity(groundtruth, message);
        return message;
    }

    void updateHorizontalPosition(
        const aircraft_msgs::msg::Groundtruth& groundtruth,
        aircraft_msgs::msg::SensorGnss& out
    ) {
        constexpr double R = 6371000.0;  // Earth radius [m]

        const Eigen::Vector2d position_ne = Eigen::Vector2d(
            _horizonal_position_dist(_gen),
            _horizonal_position_dist(_gen)
        )  * (_params.horizonal_position_noise_sd / std::numbers::sqrt2);

        const double d_lat = position_ne.x() / R;
        const double d_lon = position_ne.y() / (R * std::cos(groundtruth.latitude));

        out.latitude  = groundtruth.latitude + d_lat;
        out.longitude = groundtruth.longitude + d_lon;
    }

    void updateVerticalPosition(
        const aircraft_msgs::msg::Groundtruth& groundtruth,
        aircraft_msgs::msg::SensorGnss& out
    ) {
        out.altitude = groundtruth.altitude + _vertical_position_dist(_gen) * _params.vertical_position_noise_sd;
    }

    void updateVelocity(
        const aircraft_msgs::msg::Groundtruth& groundtruth,
        aircraft_msgs::msg::SensorGnss& out
    ) {
        const Eigen::Vector3d& velocity_body = Eigen::Vector3d(
            groundtruth.linear_velocity.x,
            groundtruth.linear_velocity.y,
            groundtruth.linear_velocity.z
        );
        const Eigen::Quaterniond& orientation = Eigen::Quaterniond(
            groundtruth.orientation.w,
            groundtruth.orientation.x,
            groundtruth.orientation.y,
            groundtruth.orientation.z
        );

        Eigen::Vector3d velocity_ned = orientation * velocity_body;
        const double noise_sd = 1.0 / std::numbers::sqrt3;

        for (int i = 0; i < 2; ++i) {
            velocity_ned[i] += _velocity_dist(_gen) * _params.velocity_noise_sd * noise_sd;
        }
        out.groundspeed = velocity_ned.head<2>().norm();
        out.course = std::atan2(velocity_ned.y(), velocity_ned.x());
        out.vz = velocity_ned.z() + _velocity_dist(_gen) * _params.velocity_noise_sd * noise_sd;
    }
};