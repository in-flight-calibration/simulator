#pragma once

#include "environment.hpp"

struct IsaModelParameters {
    double sea_level_pressure = 101325.0; // Pa
    double sea_level_temperature = 288.15; // K
    double temperature_lapse_rate = 0.0065; // K/m
    double gas_constant = 287.05; // J/(kg·K)
};

class IsaModel
{
public:
    inline static constexpr double G = 9.805;

    IsaModel(IsaModelParameters params = IsaModelParameters{})
        : _params(params) {
        update(0.0);
    }
    ~IsaModel() = default;

    void update(double altitude) {
        _temperature = _params.sea_level_temperature - _params.temperature_lapse_rate * altitude;
        _pressure = _params.sea_level_pressure * std::pow(_temperature / _params.sea_level_temperature, G / (_params.temperature_lapse_rate * _params.gas_constant));
        _air_density = _pressure / (_params.gas_constant * _temperature);
    }

    double getPressure() const {
        return _pressure;
    }

    double getTemperature() const {
        return _temperature;
    }

    double getAirDensity() const {
        return _air_density;
    }

private:
    IsaModelParameters _params;

    double _pressure;
    double _temperature;
    double _air_density;
};