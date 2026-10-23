#include "flightgear_process_node.hpp"

#include <stdexcept>

namespace {

Camera declareCamera(rclcpp::Node & node)
{
    Camera camera{
        .width = static_cast<int>(node.declare_parameter("camera.width", 1280)),
        .height = static_cast<int>(node.declare_parameter("camera.height", 720)),
        .fps = static_cast<int>(node.declare_parameter("camera.fps", 30)),
        .fov = static_cast<float>(node.declare_parameter("camera.fov", 75.0)),
        .roll = static_cast<float>(node.declare_parameter("camera.roll", 0.0)),
        .pitch = static_cast<float>(node.declare_parameter("camera.pitch", 0.0)),
        .yaw = static_cast<float>(node.declare_parameter("camera.yaw", 0.0))
    };

    if (camera.width <= 0 || camera.height <= 0 || camera.fps <= 0) {
        throw std::runtime_error("Camera width, height, and fps must be positive");
    }

    return camera;
}

}  // namespace

FlightgearProcessNode::FlightgearProcessNode()
    : Node("flightgear_process_node"),
      _camera(declareCamera(*this)),
      _port_control(static_cast<int>(declare_parameter("fg.port.control", 5500))),
      _port_video(static_cast<int>(declare_parameter("gst.multicast_port", 5501))),
      _port_props(static_cast<int>(declare_parameter("fg.port.props", 5502))),
      _multicast_group(declare_parameter("gst.multicast_group", "239.255.0.1")),
      _multicast_iface(declare_parameter("gst.multicast_iface", "127.0.0.1")),
      _startup_delay(rclcpp::Duration::from_seconds(
          declare_parameter("fg.control.startup_delay", 10.0)))
{
}

FlightgearProcessNode::~FlightgearProcessNode()
{
    _process.stop();
}

void FlightgearProcessNode::run()
{
    const Process::Environment env = {
        {"FGFS_WIDTH", std::to_string(_camera.width)},
        {"FGFS_HEIGHT", std::to_string(_camera.height)},
        {"FGFS_FOV", std::to_string(_camera.fov)},
        {"FGFS_FPS", std::to_string(_camera.fps)},
        {"FGFS_PORT_CONTROL", std::to_string(_port_control)},
        {"FGFS_PORT_VIDEO", std::to_string(_port_video)},
        {"FGFS_PORT_PROPS", std::to_string(_port_props)},
        {"FGFS_MULTICAST_GROUP", _multicast_group},
        {"FGFS_MULTICAST_IFACE", _multicast_iface}
    };

    if (!_process.run({SCRIPTS_DIR "/fgfs.sh"}, env)) {
        RCLCPP_ERROR(get_logger(), "Failed to start FlightGear");
        return;
    }

    get_clock()->sleep_for(_startup_delay);
}
