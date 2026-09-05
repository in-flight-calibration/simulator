#include "flightgear_control_node.hpp"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <cmath>
#include <stdexcept>

#include <tf2/LinearMath/Quaternion.hpp>
#include <tf2/LinearMath/Matrix3x3.hpp>

FlightgearControlNode::FlightgearControlNode(
    const std::string& position_topic,
    const std::string& orientation_topic,
    std::uint16_t port,
    std::chrono::microseconds interval)
    : Node("flightgear_control_node"), _port{port}, _interval{interval}
{
    _position_sub =
        create_subscription<geographic_msgs::msg::GeoPoint>(
            position_topic,
            rclcpp::SensorDataQoS(),
            std::bind(
                &FlightgearControlNode::positionCallback,
                this,
                std::placeholders::_1));

    _orientation_sub =
        create_subscription<geometry_msgs::msg::Quaternion>(
            orientation_topic,
            rclcpp::SensorDataQoS(),
            std::bind(
                &FlightgearControlNode::orientationCallback,
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
        "Flightgear Control Node started: UDP port=%u, position topic=%s, orientation topic=%s",
        static_cast<unsigned>(_port),
        "/flightgear/position",
        "/flightgear/orientation");
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

void FlightgearControlNode::positionCallback(
    const geographic_msgs::msg::GeoPoint::SharedPtr msg)
{
    // RCLCPP_INFO(
    //     get_logger(),
    //     "Position callback received"
    // );

    _packet.latitude = msg->latitude;
    _packet.longitude = msg->longitude;
    _packet.altitude_ft = msg->altitude * M_TO_FT;
    _packet_updated = true;
}

void FlightgearControlNode::orientationCallback(
    const geometry_msgs::msg::Quaternion::SharedPtr msg)
{
    // RCLCPP_INFO(
    //     get_logger(),
    //     "Orientation callback received"
    // );

    const tf2::Quaternion q(
        msg->x,
        msg->y,
        msg->z,
        msg->w);

    double roll, pitch, yaw;
    tf2::Matrix3x3(q).getRPY(roll, pitch, yaw);

    _packet.roll_deg = roll * RAD_TO_DEG;
    _packet.pitch_deg = pitch * RAD_TO_DEG;
    _packet.heading_deg = yaw * RAD_TO_DEG;
    _packet_updated = true;

    RCLCPP_INFO(
        get_logger(),
        "Orientation updated: roll=%.2f, pitch=%.2f, heading=%.2f",
        _packet.roll_deg,
        _packet.pitch_deg,
        _packet.heading_deg
    );
}

void FlightgearControlNode::timerCallback()
{
    if (_socket < 0 || !_packet_updated) {
        return;
    }

    RCLCPP_INFO(
        get_logger(),
        "Sending packet to Flightgear"
    );

    ::sendto(
        _socket,
        reinterpret_cast<const void*>(&_packet),
        sizeof(_packet),
        0,
        reinterpret_cast<const sockaddr*>(&_destination),
        sizeof(_destination));

    _packet_updated = false;
}
