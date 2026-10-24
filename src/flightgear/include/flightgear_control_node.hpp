#pragma once

#include <rclcpp/rclcpp.hpp>
#include <px4_msgs/msg/vehicle_attitude.hpp>
#include <px4_msgs/msg/vehicle_global_position.hpp>

#include <cstdint>
#include <mutex>
#include <netinet/in.h>
#include <string>

#include <tf2_eigen/tf2_eigen.hpp>
#include <tf2_geometry_msgs/tf2_geometry_msgs.hpp>
#include <tf2/LinearMath/Quaternion.hpp>
#include <tf2/LinearMath/Matrix3x3.hpp>

#include "camera.hpp"

class FlightgearControlNode : public rclcpp::Node
{
public:
    explicit FlightgearControlNode(const Camera& camera);

    ~FlightgearControlNode() override;

    void run();

private:
    const Camera& _camera;
    std::uint16_t _port;
    rclcpp::Duration _interval;
    tf2::Quaternion _camera_orientation;

    int _socket{-1};
    sockaddr_in _destination{};

    rclcpp::Subscription<px4_msgs::msg::VehicleAttitude>::SharedPtr _att_sub;
    rclcpp::Subscription<px4_msgs::msg::VehicleGlobalPosition>::SharedPtr _vgp_sub;

    rclcpp::TimerBase::SharedPtr _timer;

    struct __attribute__((packed)) Packet
    {
        double latitude = 52.2669725;
        double longitude = 20.9239028;
        double altitude_ft = 1000.0;
        double roll_deg;
        double pitch_deg;
        double heading_deg;
    } _packet;

    bool _initialized{false};
    bool _att_updated{false};
    bool _vgp_updated{false};

    std::mutex _mutex;

    bool openSocket();
    bool closeSocket();

    void attitudeCallback(
        const px4_msgs::msg::VehicleAttitude::SharedPtr msg);
    void globalPositionCallback(
        const px4_msgs::msg::VehicleGlobalPosition::SharedPtr msg);
    void timerCallback();

    static constexpr double M_TO_FT = 3.28083989501312;
    static constexpr double RAD_TO_DEG = 180.0 / M_PI;
};
