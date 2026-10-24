#include "flightgear_video_node.hpp"

#include <cv_bridge/cv_bridge.hpp>

#include <format>
#include <utility>

FlightgearVideoNode::FlightgearVideoNode(const Camera& camera)
    : Node("flightgear_video_node"),
      _camera(camera),
      _udp_port(static_cast<std::uint16_t>(declare_parameter("gst.multicast_port", 5501))),
      _topic(declare_parameter("fg.image.fg_image_topic", "/flightgear/image")),
      _multicast_group(declare_parameter("gst.multicast_group", "239.255.0.1")),
      _multicast_iface(declare_parameter("gst.multicast_iface", "lo")),
      _update_interval(rclcpp::Duration::from_seconds(1.0 / _camera.fps)),
      _stats_interval(rclcpp::Duration::from_seconds(
          declare_parameter("fg.image.stats_interval", 5.0)))
{
    _image_pub = image_transport::create_publisher(
        *this,
        _topic,
        rclcpp::SensorDataQoS()
    );
}

FlightgearVideoNode::~FlightgearVideoNode()
{
    _running = false;

    if (_cap.isOpened()) {
        _cap.release();
    }

    if (_capture_thread.joinable()) {
        _capture_thread.join();
    }
}

void FlightgearVideoNode::run()
{
    if (!openPipeline()) {
        RCLCPP_ERROR(
            get_logger(),
            "Failed to open GStreamer UDP stream on port %u",
            static_cast<unsigned>(_udp_port));
        return;
    }

    _frames_received.store(0, std::memory_order_relaxed);
    const double stats_seconds = _stats_interval.seconds();
    _stats_timer = rclcpp::create_timer(
        this,
        get_clock(),
        _stats_interval,
        [this, stats_seconds]() {
            const auto captured = _frames_received.exchange(
                0, std::memory_order_relaxed) / stats_seconds;
            const auto sent = _frames_sent.exchange(
                0, std::memory_order_relaxed) / stats_seconds;

            RCLCPP_INFO(
                get_logger(),
                "Capture: %.2f, Sent: %.2f FPS",
                captured,
                sent);
    });

    _running = true;
    _capture_thread = std::thread(
        &FlightgearVideoNode::captureLoop,
        this);

    _timer = rclcpp::create_timer(
        this,
        get_clock(),
        _update_interval,
        std::bind(&FlightgearVideoNode::update, this));

    RCLCPP_INFO(
        get_logger(),
        "Video Node started: UDP port=%u, topic=%s",
        static_cast<unsigned>(_udp_port),
        _topic.c_str());
}

bool FlightgearVideoNode::openPipeline()
{
    const auto pipeline_string = std::format(
        "udpsrc multicast-group={} "
        "auto-multicast=true "
        "multicast-iface={} "
        "port={} "
        "! application/x-rtp,"
        "media=video,"
        "encoding-name=H264,"
        "payload=96 "
        "! rtph264depay "
        "! h264parse "
        "! avdec_h264 "
        "! videoconvert "
        "! appsink "
        "sync=false "
        "drop=true "
        "max-buffers=1",
        _multicast_group,
        _multicast_iface,
        _udp_port);

    RCLCPP_INFO(
        get_logger(),
        "Opening GStreamer pipeline: %s",
        pipeline_string.c_str());

    return _cap.open(
        pipeline_string,
        cv::CAP_GSTREAMER);
}

void FlightgearVideoNode::captureLoop()
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

void FlightgearVideoNode::update()
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
