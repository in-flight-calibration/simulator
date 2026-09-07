#pragma once

#include "aircraft.hpp"

AircraftParameters smallUav() {
    AircraftParameters aircraft_params;

    aircraft_params.rigid_body.mass = 1.5;

    aircraft_params.rigid_body.inertia <<
        0.030, 0.000, 0.000,
        0.000, 0.045, 0.000,
        0.000, 0.000, 0.065;

    aircraft_params.C0 <<
        -0.035,
         0.000,
        -0.300,
         0.000,
        -0.050,
         0.000;

    aircraft_params.Cab <<
         0.20,  0.10,  0.00,  0.00,  0.00,
         0.00,  0.00, -0.70,  0.00, -0.05,
        -4.00,  0.30,  0.00,  0.00,  0.00,
         0.00,  0.00, -0.05,  0.00,  0.00,
        -1.00,  0.00,  0.00, -0.05,  0.00,
         0.00,  0.00,  0.15,  0.00,  0.00;

    aircraft_params.Cpqr <<
         0.00, -0.10,  0.00,
         0.00,  0.00,  0.30,
         0.00, -8.00,  0.00,
        -0.45,  0.00,  0.10,
         0.00, -9.00,  0.00,
         0.00,  0.00, -0.12;

    aircraft_params.Cdelta <<
         0.00,  0.00,  0.00,
         0.00,  0.00,  0.00,
         0.00,  0.00,  0.00,
         0.04,  0.00,  0.00,
         0.00,  0.01,  0.00,
         0.00,  0.00,  0.02;

    aircraft_params.surface_area   = 0.14;
    aircraft_params.wing_span      = 1.00;
    aircraft_params.mean_chord     = 0.14;

    aircraft_params.thrust_max     = 12.0;

    aircraft_params.launcher.force = 30.0;
    aircraft_params.launcher.time  = 0.7;
    return aircraft_params;
}