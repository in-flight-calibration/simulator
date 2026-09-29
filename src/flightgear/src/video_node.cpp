#include "video_node.hpp"

#include <cv_bridge/cv_bridge.hpp>

#include <iostream>
#include <sstream>
#include <utility>

VideoNode::VideoNode(
    std::uint16_t udp_port,
    const std::string& topic,
    std::chrono::milliseconds update_interval)
    : Node("video_node"),
      _udp_port(udp_port),
      _topic(topic),
      _update_interval(update_interval)
{
    _image_pub = image_transport::create_publisher(
        *this,
        _topic,
        rclcpp::SensorDataQoS()
    );
}

VideoNode::~VideoNode()
{
    _running = false;

    if (_cap.isOpened()) {
        _cap.release();
    }

    if (_capture_thread.joinable()) {
        _capture_thread.join();
    }
}

void VideoNode::run()
{
    if (!openPipeline()) {
        RCLCPP_ERROR(
            get_logger(),
            "Failed to open GStreamer UDP stream on port %u",
            static_cast<unsigned>(_udp_port));
        return;
    }

    _running = true;
    _capture_thread = std::thread(
        &VideoNode::captureLoop,
        this);

    _timer = create_wall_timer(
        _update_interval,
        std::bind(&VideoNode::update, this));

    RCLCPP_INFO(
        get_logger(),
        "Video Node started: UDP port=%u, topic=%s",
        static_cast<unsigned>(_udp_port),
        _topic.c_str());
}

bool VideoNode::openPipeline()
{
    std::ostringstream pipeline;

    pipeline
        << "udpsrc multicast-group=239.255.0.1 "
        "auto-multicast=true "
        << "port=" << _udp_port
        << " ! application/x-rtp,"
        "media=video,"
        "encoding-name=H264,"
        "payload=96 "
        << "! rtph264depay "
        << "! avdec_h264 "
        << "! videoconvert "
        << "! appsink "
        "sync=false "
        "drop=true "
        "max-buffers=1";

    const auto pipeline_string = pipeline.str();

    RCLCPP_INFO(
        get_logger(),
        "Opening GStreamer pipeline: %s",
        pipeline_string.c_str());

    return _cap.open(
        pipeline_string,
        cv::CAP_GSTREAMER);
}

void VideoNode::captureLoop()
{
    while (_running && rclcpp::ok()) {
        cv::Mat frame;

        if (!_cap.read(frame) || frame.empty()) {
            continue;
        }

        {
            std::lock_guard<std::mutex> lock(_frame_mutex);
            _latest_frame = std::move(frame);
        }
    }
}

void VideoNode::update()
{
    cv::Mat frame;

    {
        std::lock_guard<std::mutex> lock(_frame_mutex);

        if (_latest_frame.empty()) {
            return;
        }

        cv::resize(
            _latest_frame,
            frame,
            cv::Size(IMAGE_WIDTH, IMAGE_HEIGHT));
    }

    auto msg = cv_bridge::CvImage(
        std_msgs::msg::Header{},
        "bgr8",
        frame).toImageMsg();

    msg->header.stamp = now();

    _image_pub.publish(msg);
}
