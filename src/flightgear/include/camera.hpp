#pragma once

#include <opencv2/core.hpp>

class Camera
{
public:
    struct Distortion
    {
        double k1;
        double k2;
        double p1;
        double p2;
        double k3;
    };

    struct Parameters
    {
        int width;      // Camera width in pixels
        int height;     // Camera height in pixels
        int fps;        // Camera frames per second

        double roll;    // Camera roll in degrees
        double pitch;   // Camera pitch in degrees
        double yaw;     // Camera yaw in degrees

        double fov;     // Horizontal field of view in degrees
        double cx;      // Camera principal point x-coordinate
        double cy;      // Camera principal point y-coordinate

        Distortion distortion; // Camera distortion coefficients
    };

    explicit Camera(const Parameters& params);

    int getWidth() const { return _params.width; }
    int getHeight() const { return _params.height; }
    int getFps() const { return _params.fps; }
    double getFov() const { return _params.fov; }
    double getRoll() const { return _params.roll; }
    double getPitch() const { return _params.pitch; }
    double getYaw() const { return _params.yaw; }
    double getCx() const { return _params.cx; }
    double getCy() const { return _params.cy; }
    const Distortion& getDistortion() const { return _params.distortion; }

    cv::Size getRenderSize() const { return _render_size; }
    double getRenderFov() const { return _render_fov; }

    const cv::Mat& getPixelMap() const { return _pixel_map; }

private:
    void validateParameters() const;
    void calculateIntrinsics();
    void calculatePixelMap();
    void calculateRenderSize();
    void calculateRenderFov();
    void centerPixelMap();

    Parameters _params;

    double _f;          // Focal length in pixels
    cv::Matx33d _K;     // Intrinsic matrix

    cv::Size _render_size;
    double _render_fov;

    cv::Mat _pixel_map;
};
