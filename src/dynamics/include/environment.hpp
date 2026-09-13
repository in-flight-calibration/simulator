#pragma once

#include <Eigen/Dense>

#include "isa.hpp"
#include "wind.hpp"

#include "aircraft_msgs/msg/environment.hpp"

class Environment
{
public:
    inline static const Eigen::Vector3d G = Eigen::Vector3d(0.0, 0.0, IsaModel::G);

    Environment(rclcpp::Node& parent, std::string environment_topic)
        : _parent(parent)
    {
        _environment_pub = _parent.create_publisher<aircraft_msgs::msg::Environment>(environment_topic, 10);
    }

    void update(double time, double altitude) {
        const double dt = time - _last_time;
        _last_time = time;

        _isa_model.update(altitude);
        _wind_model.update(dt, altitude);
        publish();
    }

    double getPressure() const {
        return _isa_model.getPressure();
    }

    double getTemperature() const {
        return _isa_model.getTemperature();
    }

    double getAirDensity() const {
        return _isa_model.getAirDensity();
    }

    Eigen::Vector3d getWind() const {
        return _wind_model.get();
    }

private:
    rclcpp::Node& _parent;
    rclcpp::Publisher<aircraft_msgs::msg::Environment>::SharedPtr _environment_pub;
    
    double _last_time = 0.0;

    IsaModel _isa_model;
    WindModel _wind_model;

    void publish() {
        aircraft_msgs::msg::Environment msg;
        msg.header.stamp = _parent.now();

        msg.pressure = getPressure();
        msg.temperature = getTemperature();
        msg.air_density = getAirDensity();

        const Eigen::Vector3d wind = getWind();
        msg.wind.x = wind.x();
        msg.wind.y = wind.y();
        msg.wind.z = wind.z();

        _environment_pub->publish(msg);
    }
};