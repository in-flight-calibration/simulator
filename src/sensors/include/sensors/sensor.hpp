#pragma once

#include "rclcpp/rclcpp.hpp"
#include <Eigen/Dense>

#include "aircraft_msgs/msg/environment.hpp"
#include "aircraft_msgs/msg/groundtruth.hpp"

template<typename MessageT, typename ParamsT>
class Sensor
{
public:
    Sensor(rclcpp::Node& parent, const ParamsT& params, std::string topic_name, double update_time = 0.0) 
        : _parent(parent), _params(params), _update_time(update_time) {
        _publisher = _parent.create_publisher<MessageT>(topic_name, 10);
    }

    virtual ~Sensor() = default;

    void update(const aircraft_msgs::msg::Groundtruth& groundtruth,
                const aircraft_msgs::msg::Environment& environment) {
        if (groundtruth.time < _last_update_time + _update_time) {
            return;
        }

        const double dt = groundtruth.time - _last_update_time;
        auto message = process(dt, groundtruth, environment);
        _publisher->publish(message);
        _last_update_time = groundtruth.time;
    }

protected:
    rclcpp::Node& _parent;

    ParamsT _params;
    rclcpp::Publisher<MessageT>::SharedPtr _publisher;

    const double _update_time;
    double _last_update_time = 0.0;

    virtual MessageT process(
        double dt, 
        const aircraft_msgs::msg::Groundtruth& groundtruth,
        const aircraft_msgs::msg::Environment& environment
    ) = 0;
};