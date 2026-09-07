# Ground truth aircraft state.

std_msgs/Header header

# Position in local/world frame [m].
geometry_msgs/Point position

# Aircraft orientation as quaternion.
geometry_msgs/Quaternion orientation

# Linear velocity in body frame [m/s].
geometry_msgs/Vector3 linear_velocity

# Angular velocity in body frame [rad/s].
geometry_msgs/Vector3 angular_velocity

# Air-relative velocity in body frame [m/s].
geometry_msgs/Vector3 airspeed

# Global position in WGS84.
float64 latitude       # [deg]
float64 longitude      # [deg]
float64 altitude       # [m]
