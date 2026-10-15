#include "flightgear_control_node.hpp"
#include "process.hpp"
#include "video_node.hpp"

#include <rclcpp/rclcpp.hpp>
#include <yaml-cpp/yaml.h>

Process start_fgfs(const Camera &camera)
{
    const Process::Environment env = {
        {"FGFS_WIDTH", std::to_string(camera.width)},
        {"FGFS_HEIGHT", std::to_string(camera.height)},
        {"FGFS_FOV", std::to_string(camera.fov)},
        {"FGFS_FPS", std::to_string(camera.fps)},
        {"FGFS_PORT_CONTROL", "5500"},
        {"FGFS_PORT_VIDEO", "5501"}
    };

    Process process;

    if (!process.run({SCRIPTS_DIR "/fgfs.sh"}, env)) {
        return {};
    }

    return process;
}

int main(int argc, char * argv[])
{
    rclcpp::init(argc, argv);

    if (argc < 2) {
        throw std::runtime_error("Usage: flightgear <config.yaml>");
    }

    const std::string config_file = std::string(argv[1]);
    const YAML::Node config = YAML::LoadFile(config_file);
    const auto cam = config["camera"];

    Camera camera{
        .width = cam["width"].as<int>(),
        .height = cam["height"].as<int>(),
        .fps = cam["fps"].as<int>(),
        .fov = cam["fov"].as<float>(),
        .roll = cam["roll"].as<float>(),
        .pitch = cam["pitch"].as<float>(),
        .yaw = cam["yaw"].as<float>()
    };

    if (camera.width <= 0 || camera.height <= 0 || camera.fps <= 0) {
        throw std::runtime_error("Camera width, height, and fps must be positive");
    }

    Process process = start_fgfs(camera);

    auto flightgear_control_node = std::make_shared<FlightgearControlNode>(
        camera,
        "/fmu/out/vehicle_attitude",
        "/fmu/out/vehicle_global_position_groundtruth",
        5500
    );

    auto video_node = std::make_shared<VideoNode>(
        camera,
        "/flightgear/image",
        5501
    );

    flightgear_control_node->run();
    video_node->run();

    rclcpp::executors::MultiThreadedExecutor executor;
    executor.add_node(flightgear_control_node);
    executor.add_node(video_node);

    std::cout << "Flightgear running..." << std::endl;
    executor.spin();

    rclcpp::shutdown();
    process.stop();
    return 0;
}
