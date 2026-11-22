#include "flightgear_process_node.hpp"

namespace {

Camera declareCamera(rclcpp::Node & node)
{
    return Camera({
        .width = static_cast<int>(node.declare_parameter("camera.width", 1280)),
        .height = static_cast<int>(node.declare_parameter("camera.height", 720)),
        .fps = static_cast<int>(node.declare_parameter("camera.fps", 30)),
        .roll = node.declare_parameter("camera.roll", 0.0),
        .pitch = node.declare_parameter("camera.pitch", 0.0),
        .yaw = node.declare_parameter("camera.yaw", 0.0),
        .fov = node.declare_parameter("camera.fov", 75.0),
        .cx = node.declare_parameter("camera.cx", 0.0),
        .cy = node.declare_parameter("camera.cy", 0.0),
        .distortion = {
            node.declare_parameter("camera.distortion.k1", 0.0),
            node.declare_parameter("camera.distortion.k2", 0.0),
            node.declare_parameter("camera.distortion.p1", 0.0),
            node.declare_parameter("camera.distortion.p2", 0.0),
            node.declare_parameter("camera.distortion.k3", 0.0)
        }
    });
}

}  // namespace

FlightgearProcessNode::FlightgearProcessNode()
    : Node("flightgear_process_node"),
      _camera(declareCamera(*this)),
      _port_control(static_cast<int>(declare_parameter("fg.port.control", 5500))),
      _port_video(static_cast<int>(declare_parameter("gst.multicast_port", 5501))),
      _multicast_group(declare_parameter("gst.multicast_group", "239.255.0.1")),
      _multicast_iface(declare_parameter("gst.multicast_iface", "lo")),
      _video_delay(static_cast<int>(declare_parameter("gst.video_delay_ms", 0))),
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
        {"FGFS_WIDTH", std::to_string(_camera.getRenderSize().width)},
        {"FGFS_HEIGHT", std::to_string(_camera.getRenderSize().height)},
        {"FGFS_FOV", std::to_string(_camera.getRenderFov())},
        {"FGFS_FPS", std::to_string(_camera.getFps())},
        {"FGFS_PORT_CONTROL", std::to_string(_port_control)},
        {"FGFS_PORT_VIDEO", std::to_string(_port_video)},
        {"FGFS_MULTICAST_GROUP", _multicast_group},
        {"FGFS_MULTICAST_IFACE", _multicast_iface},
        {"FGFS_VIDEO_DELAY_MS", std::to_string(_video_delay)}
    };

    if (!_process.run({SCRIPTS_DIR "/fgfs.sh"}, env)) {
        RCLCPP_ERROR(get_logger(), "Failed to start FlightGear");
        return;
    }

    get_clock()->sleep_for(_startup_delay);
}
