#include "flightgear_control_node.hpp"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <cmath>
#include <stdexcept>

#include <tf2_eigen/tf2_eigen.hpp>
#include <tf2_geometry_msgs/tf2_geometry_msgs.hpp>
#include <tf2/LinearMath/Quaternion.hpp>
#include <tf2/LinearMath/Matrix3x3.hpp>

FlightgearControlNode::FlightgearControlNode(
    const std::string& groundtruth_topic,
    std::uint16_t port,
    std::chrono::microseconds interval)
    : Node("flightgear_control_node"), _port{port}, _interval{interval}
{
    _groundtruth_sub =
        create_subscription<aircraft_msgs::msg::Groundtruth>(
            groundtruth_topic,
            rclcpp::SensorDataQoS(),
            std::bind(
                &FlightgearControlNode::groundtruthCallback,
                this,
                std::placeholders::_1));
}

FlightgearControlNode::~FlightgearControlNode()
{
    closeSocket();
}

void FlightgearControlNode::run()
{
    if (!openSocket()) {
        return;
    }

    _timer = create_wall_timer(
        _interval,
        std::bind(&FlightgearControlNode::timerCallback, this)
    );

    RCLCPP_INFO(
        get_logger(),
        "Flightgear Control Node started: UDP port=%u",
        static_cast<unsigned>(_port)
    );
}

bool FlightgearControlNode::openSocket()
{
    _socket = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (_socket < 0)
        throw std::runtime_error("Failed to create UDP socket");

    _destination.sin_family = AF_INET;
    _destination.sin_port = htons(_port);

    if (::inet_pton(
            AF_INET,
            "127.0.0.1",
            &_destination.sin_addr) != 1)
    {
        ::close(_socket);
        _socket = -1;
        return false;
    }

    return true;
}

bool FlightgearControlNode::closeSocket()
{
    if (_socket >= 0) {
        ::close(_socket);
        _socket = -1;
        return true;
    }

    return false;
}

void FlightgearControlNode::groundtruthCallback(
    const aircraft_msgs::msg::Groundtruth::SharedPtr msg)
{
    _packet.latitude = msg->latitude;
    _packet.longitude = msg->longitude;
    _packet.altitude_ft = msg->altitude * M_TO_FT;
    
    tf2::Quaternion q;
    double roll, pitch, yaw;
    tf2::fromMsg(msg->orientation, q);
    tf2::Matrix3x3(q).getRPY(roll, pitch, yaw);

    _packet.roll_deg = roll * RAD_TO_DEG;
    _packet.pitch_deg = pitch * RAD_TO_DEG;
    _packet.heading_deg = yaw * RAD_TO_DEG;
    _packet_updated = true;
}

void FlightgearControlNode::timerCallback()
{
    if (_socket < 0 || !_packet_updated) {
        return;
    }

    ::sendto(
        _socket,
        reinterpret_cast<const void*>(&_packet),
        sizeof(_packet),
        0,
        reinterpret_cast<const sockaddr*>(&_destination),
        sizeof(_destination));

    _packet_updated = false;
}
