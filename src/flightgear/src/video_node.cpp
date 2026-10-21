#include "video_node.hpp"

#include <cv_bridge/cv_bridge.hpp>

#include <iostream>
#include <sstream>
#include <utility>

VideoNode::VideoNode(
    const Camera& camera,
    const std::string& topic,
    std::uint16_t udp_port)
    : Node("video_node"),
      _camera(camera),
      _udp_port(udp_port),
      _topic(topic),
      _update_interval(std::chrono::milliseconds(1000 / camera.fps))
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

    _frames_received.store(0, std::memory_order_relaxed);
    _stats_timer = create_wall_timer(
        std::chrono::seconds(
            static_cast<int>(std::round(STATS_INTERVAL_SECONDS))),
        [this]() {
            const auto captured = _frames_received.exchange(
                0, std::memory_order_relaxed) / STATS_INTERVAL_SECONDS;
            const auto sent = _frames_sent.exchange(
                0, std::memory_order_relaxed) / STATS_INTERVAL_SECONDS;

            RCLCPP_INFO(
                get_logger(),
                "Capture: %.2f, Sent: %.2f FPS",
                static_cast<double>(captured),
                static_cast<double>(sent));
    });

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
        << "! h264parse "
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

        _frames_received.fetch_add(1, std::memory_order_relaxed);

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
            cv::Size(_camera.width, _camera.height));
    }

    auto msg = cv_bridge::CvImage(
        std_msgs::msg::Header{},
        "bgr8",
        frame).toImageMsg();
    msg->header.stamp = now();
    _image_pub.publish(msg);
    _frames_sent.fetch_add(1, std::memory_order_relaxed);
}
