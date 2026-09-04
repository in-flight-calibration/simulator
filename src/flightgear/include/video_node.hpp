#pragma once

#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/image.hpp>
#include <image_transport/image_transport.hpp>

#include <opencv2/opencv.hpp>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>

class VideoNode : public rclcpp::Node
{
public:
    VideoNode(
        std::uint16_t udp_port,
        const std::string& topic,
        std::chrono::milliseconds update_interval
    );

    ~VideoNode() override;

    void run();

private:
    bool openPipeline();
    void captureLoop();
    void update();

    std::uint16_t _udp_port;
    std::string _topic;
    std::chrono::milliseconds _update_interval;

    cv::VideoCapture _cap;

    std::mutex _frame_mutex;
    cv::Mat _latest_frame;
    std::thread _capture_thread;
    std::atomic<bool> _running{false};

    rclcpp::TimerBase::SharedPtr _timer;
    image_transport::Publisher _image_pub;
};
