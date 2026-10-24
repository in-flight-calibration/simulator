#pragma once

#include <rclcpp/rclcpp.hpp>

#include <string>

#include "camera.hpp"
#include "process.hpp"

class FlightgearProcessNode : public rclcpp::Node
{
public:
    FlightgearProcessNode();
    ~FlightgearProcessNode() override;

    void run();

    const Camera& camera() const { return _camera; }

private:
    Camera _camera;
    int _port_control;
    int _port_video;
    std::string _multicast_group;
    std::string _multicast_iface;
    rclcpp::Duration _startup_delay;

    Process _process;
};
