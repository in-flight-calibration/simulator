#pragma once

#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/image.hpp>
#include <image_transport/image_transport.hpp>

#include <opencv2/opencv.hpp>

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>

#include "camera.hpp"

class FlightgearVideoNode : public rclcpp::Node
{
public:
    explicit FlightgearVideoNode(const Camera& camera);

    ~FlightgearVideoNode() override;

    void run();

private:
    const Camera& _camera;

    bool openPipeline();
    void captureLoop();
    void update();

    std::uint16_t _udp_port;
    std::string _topic;
    std::string _multicast_group;
    std::string _multicast_iface;
    rclcpp::Duration _update_interval;
    rclcpp::Duration _stats_interval;

    cv::VideoCapture _cap;

    std::mutex _frame_mutex;
    cv::Mat _latest_frame;
    std::thread _capture_thread;
    std::atomic<bool> _running{false};

    rclcpp::TimerBase::SharedPtr _timer;
    image_transport::Publisher _image_pub;

    std::atomic<std::uint64_t> _frames_received{0};
    std::atomic<std::uint64_t> _frames_sent{0};
    rclcpp::TimerBase::SharedPtr _stats_timer;
};
