/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "moving_average.h"

#include <tl/expected.hpp>

struct device;

/// Bundles a temperature/humidity sensor with its smoothing state so that the
/// moving averages always travel with the device they belong to. The primary
/// sensor (the one driving Matter cluster updates) is selected by holding a
/// pointer to one of these and swapping which sensor is "primary" at runtime.
struct Sensor {
	const device *dev  = nullptr;
	const char   *name = nullptr;

	bool IsAvailable() const { return dev != nullptr; }

	static constexpr int16_t  kTemperatureInvalid = 0x8000;
	static constexpr uint16_t kHumidityInvalid    = 0xffff;

	struct Readings {
		int16_t  temperature;
		uint16_t humidity;
	};

	tl::expected<Readings, int> Read();

	// alpha = 20/32 = 0.625
	MovingAverage<int16_t>  temperatureAverage{ 20 };
	MovingAverage<uint16_t> humidityAverage   { 20 };
};
