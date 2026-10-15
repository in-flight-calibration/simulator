#pragma once

struct Camera
{
    int width; // Camera width in pixels
    int height; // Camera height in pixels
    int fps; // Camera frames per second

    float fov; // Camera field of view in degrees

    float roll; // Camera roll in degrees
    float pitch; // Camera pitch in degrees
    float yaw; // Camera yaw in degrees
};
