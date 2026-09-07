#pragma once

#include <rclcpp/rclcpp.hpp>
#include <aircraft_msgs/msg/groundtruth.hpp>

#include <chrono>
#include <cstdint>
#include <netinet/in.h>
#include <string>

class FlightgearControlNode : public rclcpp::Node
{
public:
    FlightgearControlNode(
        const std::string& groundtruth_topic,
        std::uint16_t port,
        std::chrono::microseconds interval);

    ~FlightgearControlNode() override;

    void run();

private:
    std::uint16_t _port;
    std::chrono::microseconds _interval;

    int _socket{-1};
    sockaddr_in _destination{};

    rclcpp::Subscription<aircraft_msgs::msg::Groundtruth>::SharedPtr
        _groundtruth_sub;

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

    bool _packet_updated{false};

    bool openSocket();
    bool closeSocket();

    void groundtruthCallback(
        const aircraft_msgs::msg::Groundtruth::SharedPtr msg);
    void timerCallback();

    static constexpr double M_TO_FT = 3.28083989501312;
    static constexpr double RAD_TO_DEG = 180.0 / M_PI;
};
