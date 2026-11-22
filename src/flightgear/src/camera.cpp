#include "camera.hpp"

#include <opencv2/calib3d.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <stdexcept>
#include <string>
#include <vector>

Camera::Camera(const Parameters& params)
    : _params(params)
{
    validateParameters();
    calculateIntrinsics();
    calculatePixelMap();
    calculateRenderSize();
    calculateRenderFov();
    centerPixelMap();
}

void Camera::validateParameters() const
{
    const auto& p = _params;
    const auto& d = p.distortion;

    if (p.width <= 0 || p.height <= 0 || p.fps <= 0) {
        throw std::invalid_argument("Camera width, height, and fps must be positive");
    }

    if (p.width < p.height) {
        throw std::invalid_argument("Camera width must not be smaller than height");
    }

    if (!(p.fov >= 0.1 && p.fov <= 179.9)) {
        throw std::invalid_argument("Camera FOV must be in [0.1, 179.9] degrees");
    }

    for (double value : {p.cx, p.cy, d.k1, d.k2, d.p1, d.p2, d.k3}) {
        if (!std::isfinite(value)) {
            throw std::invalid_argument("Camera principal point and distortion must be finite");
        }
    }
}

void Camera::calculateIntrinsics()
{
    const double fov_rad = _params.fov * std::numbers::pi / 180.0;
    const double cx = (_params.width - 1) / 2.0 + _params.cx;
    const double cy = (_params.height - 1) / 2.0 + _params.cy;

    _f = _params.width / (2 * std::tan(fov_rad / 2.0));
    _K = cv::Matx33d(
        _f, 0, cx,
        0, _f, cy,
        0, 0, 1);
}

void Camera::calculatePixelMap()
{
    const double tolerance = 0.01;
    const auto& d = _params.distortion;
    const std::array coefficients{d.k1, d.k2, d.p1, d.p2, d.k3};

    std::vector<cv::Point2d> pixels;
    pixels.reserve(static_cast<std::size_t>(_params.width) * _params.height);

    for (int v = 0; v < _params.height; ++v) {
        for (int u = 0; u < _params.width; ++u) {
            pixels.emplace_back(u, v);
        }
    }

    std::vector<cv::Point2d> rays;
    cv::undistortPoints(pixels, rays, _K, coefficients, cv::noArray(), cv::noArray(),
                        {cv::TermCriteria::COUNT | cv::TermCriteria::EPS, 50, 1e-6});

    // OpenCV does not report inverse failures; verify the rays land back on their pixels.
    std::vector<cv::Point3d> points;
    std::vector<cv::Point2d> check;
    cv::convertPointsToHomogeneous(rays, points);
    cv::projectPoints(points, cv::Vec3d::all(0), cv::Vec3d::all(0), _K, coefficients, check);

    _pixel_map.create(_params.height, _params.width, CV_32FC2);
    auto* map = _pixel_map.ptr<cv::Vec2f>();

    for (std::size_t i = 0; i < pixels.size(); ++i) {
        if (!(cv::norm(check[i] - pixels[i]) < tolerance)) {
            throw std::invalid_argument("Cannot invert camera distortion over the output image");
        }

        map[i] = cv::Vec2f(_f * rays[i].x, _f * rays[i].y);
    }
}

void Camera::calculateRenderSize()
{
    double extent_x = (_params.width - 1) / 2.0;
    double extent_y = (_params.height - 1) / 2.0;

    for (const auto& offset : cv::Mat_<cv::Vec2f>(_pixel_map)) {
        extent_x = std::max<double>(extent_x, std::abs(offset[0]));
        extent_y = std::max<double>(extent_y, std::abs(offset[1]));
    }

    const double width = 2 * std::ceil(extent_x + 0.5);
    const double height = 2 * std::ceil(extent_y + 0.5);

    if (width < height) {
        throw std::invalid_argument("Render width must not be smaller than height");
    }

    _render_size = {static_cast<int>(width), static_cast<int>(height)};
}

void Camera::calculateRenderFov()
{
    constexpr double max_fov = 179.9;
    const double render_fov_rad = 2 * std::atan(_render_size.width / (2 * _f));
    _render_fov = render_fov_rad * 180.0 / std::numbers::pi;

    if (_render_fov > max_fov) {
        throw std::invalid_argument("Required render exceeds " + std::to_string(max_fov) + "-degree FOV limit");
    }
}

void Camera::centerPixelMap()
{
    _pixel_map += cv::Scalar((_render_size.width - 1) / 2.0, (_render_size.height - 1) / 2.0);
}
