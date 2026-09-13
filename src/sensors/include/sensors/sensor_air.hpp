#pragma once

#include <random>

#include "sensors/sensor.hpp"

#include <aircraft_msgs/msg/sensor_air.hpp>

struct SensorAirParams
{
    double dynamic_pressure_bias = 0.0;
    double static_pressure_noise_sd = 0.0;

    double static_pressure_bias = 0.0;
    double dynamic_pressure_noise_sd = 0.0;

    double temperature_bias = 0.0;
    double temperature_noise_sd = 0.0;
};

class SensorAir : public Sensor<aircraft_msgs::msg::SensorAir, SensorAirParams>
{
public:
    using Sensor<aircraft_msgs::msg::SensorAir, SensorAirParams>::Sensor;

private:
    std::mt19937 _gen{std::random_device{}()};
    std::normal_distribution<double> _dynamic_pressure_dist{0.0, 1.0};
    std::normal_distribution<double> _static_pressure_dist{0.0, 1.0};
    std::normal_distribution<double> _temperature_dist{0.0, 1.0};

    aircraft_msgs::msg::SensorAir process(
        [[maybe_unused]] double dt, 
        const aircraft_msgs::msg::Groundtruth& groundtruth, 
        const aircraft_msgs::msg::Environment& environment
    ) override {
        aircraft_msgs::msg::SensorAir message;
        message.header.stamp = _parent.now();

        const Eigen::Vector3d airspeed{
            groundtruth.airspeed.x,
            groundtruth.airspeed.y,
            groundtruth.airspeed.z
        };
        const double V_sq = airspeed.squaredNorm();
        const double differential_pressure = 0.5 * environment.air_density * V_sq;

        message.differential_pressure = differential_pressure + _params.dynamic_pressure_bias + _dynamic_pressure_dist(_gen) * _params.dynamic_pressure_noise_sd;
        message.static_pressure = environment.pressure + _params.static_pressure_bias + _static_pressure_dist(_gen) * _params.static_pressure_noise_sd;
        message.temperature = environment.temperature + _params.temperature_bias + _temperature_dist(_gen) * _params.temperature_noise_sd;
        return message;
    }
};