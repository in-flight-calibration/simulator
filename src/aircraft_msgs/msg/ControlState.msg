# Aircraft control state message

std_msgs/Header header

float64 energy_error
float64 distribution_error

float64 desired_speed
float64 desired_height

float64 energy_integral
float64 distribution_integral

geometry_msgs/Vector3 desired_attitude

geometry_msgs/Vector3 desired_rates
geometry_msgs/Vector3 rates_integral