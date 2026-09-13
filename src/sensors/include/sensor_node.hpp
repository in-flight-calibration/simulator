#pragma once

#include <rclcpp/rclcpp.hpp>

#include <sensors/sensor_air.hpp>
#include <sensors/sensor_gnss.hpp>
#include <sensors/sensor_imu.hpp>
#include <sensors/sensor_ins.hpp>

struct SensorParameters
{
    SensorAirParams air_params;
    SensorGnssParams gnss_params;
    SensorImuParams imu_params;
    SensorInsParams ins_params;
};

class SensorNode : public rclcpp::Node
{
public:
    SensorNode(
        SensorParameters params,
        std::chrono::milliseconds interval,
        std::string groundtruth_topic,
        std::string environment_topic
    )
    : rclcpp::Node("sensor_node"),
      _air(*this, params.air_params, "sensors/air", 0.02),
      _gnss(*this, params.gnss_params, "sensors/gnss", 0.1),
      _imu(*this, params.imu_params, "sensors/imu", -1.0),
      _ins(*this, params.ins_params, "sensors/ins", -1.0)
    {
        _groundtruth_sub = this->create_subscription<aircraft_msgs::msg::Groundtruth>(
            groundtruth_topic,
            10,
            [this](const aircraft_msgs::msg::Groundtruth::SharedPtr msg) {
                std::lock_guard<std::mutex> lock(_mutex);
                _groundtruth = *msg;
            }
        );
        _environment_sub = this->create_subscription<aircraft_msgs::msg::Environment>(
            environment_topic,
            10,
            [this](const aircraft_msgs::msg::Environment::SharedPtr msg) {
                std::lock_guard<std::mutex> lock(_mutex);
                _environment = *msg;
            }
        );
        _timer = this->create_wall_timer(
            interval,
            std::bind(&SensorNode::timer_callback, this)
        );
    }

private:
    SensorAir _air;
    SensorGnss _gnss;
    SensorImu _imu;
    SensorIns _ins;

    std::mutex _mutex;
    aircraft_msgs::msg::Groundtruth _groundtruth;
    aircraft_msgs::msg::Environment _environment;

    rclcpp::Subscription<aircraft_msgs::msg::Groundtruth>::SharedPtr
        _groundtruth_sub;
    rclcpp::Subscription<aircraft_msgs::msg::Environment>::SharedPtr
        _environment_sub;
    rclcpp::TimerBase::SharedPtr _timer;

    auto get_data()
    {
        std::lock_guard<std::mutex> lock(_mutex);
        return std::make_tuple(_groundtruth, _environment);
    }

    void timer_callback()
    {
        auto [groundtruth, environment] = get_data();
        _air.update(groundtruth, environment);
        _gnss.update(groundtruth, environment);
        _imu.update(groundtruth, environment);
        _ins.update(groundtruth, environment);
    }
};