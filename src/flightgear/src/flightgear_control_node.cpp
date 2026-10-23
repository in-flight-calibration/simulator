#include "flightgear_control_node.hpp"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <stdexcept>

FlightgearControlNode::FlightgearControlNode(const Camera& camera)
    : Node("flightgear_control_node"),
      _camera(camera),
      _port(static_cast<std::uint16_t>(declare_parameter("fg.port.control", 5500))),
      _interval(rclcpp::Duration::from_seconds(1.0 / (2.0 * _camera.fps)))
{
    const auto vehicle_attitude_topic = declare_parameter(
        "fg.control.attitude_topic", "/fmu/out/vehicle_attitude");
    const auto vehicle_global_position_topic = declare_parameter(
        "fg.control.position_topic", "/fmu/out/vehicle_global_position_groundtruth");

    _camera_orientation.setRPY(
        _camera.roll / RAD_TO_DEG,
        _camera.pitch / RAD_TO_DEG,
        _camera.yaw / RAD_TO_DEG
    );

    _att_sub =
        create_subscription<px4_msgs::msg::VehicleAttitude>(
            vehicle_attitude_topic,
            rclcpp::SensorDataQoS(),
            std::bind(
                &FlightgearControlNode::attitudeCallback,
                this,
                std::placeholders::_1));

    _vgp_sub =
        create_subscription<px4_msgs::msg::VehicleGlobalPosition>(
            vehicle_global_position_topic,
            rclcpp::SensorDataQoS(),
            std::bind(
                &FlightgearControlNode::globalPositionCallback,
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

    _timer = rclcpp::create_timer(
        this,
        get_clock(),
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

void FlightgearControlNode::attitudeCallback(
    const px4_msgs::msg::VehicleAttitude::SharedPtr msg)
{
    tf2::Quaternion q
        {msg->q[1], msg->q[2], msg->q[3], msg->q[0]};
    double roll, pitch, yaw;
    tf2::Matrix3x3(q * _camera_orientation).getRPY(roll, pitch, yaw);

    _packet.roll_deg = roll * RAD_TO_DEG;
    _packet.pitch_deg = pitch * RAD_TO_DEG;
    _packet.heading_deg = yaw * RAD_TO_DEG;

    _packet_updated = true;
}

void FlightgearControlNode::globalPositionCallback(
    const px4_msgs::msg::VehicleGlobalPosition::SharedPtr msg)
{
    _packet.latitude = msg->lat;
    _packet.longitude = msg->lon;
    _packet.altitude_ft = msg->alt * M_TO_FT;

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
