/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "moving_average.h"

#include <tl/expected.hpp>

#include <optional>

struct device;

/// Bundles a temperature/humidity sensor with its smoothing state so that the
/// moving averages always travel with the device they belong to. The primary
/// sensor (the one driving Matter cluster updates) is selected by holding a
/// pointer to one of these and swapping which sensor is "primary" at runtime.
struct Sensor {
	Sensor() = default;

	/// Wraps @p device with the readiness check: if @p device is null or
	/// device_is_ready() returns false, the resulting Sensor reports
	/// IsAvailable() == false and logs a warning.
	Sensor(const device *device, const char *sensorName);

	const device *dev  = nullptr;
	const char   *name = nullptr;

	bool IsAvailable() const { return dev != nullptr; }

	struct Readings {
		std::optional<int16_t>  temperature;
		std::optional<uint16_t> humidity;
	};

	tl::expected<Readings, int> Read();

	// alpha = 20/32 = 0.625
	MovingAverage<int16_t>  temperatureAverage{ 20 };
	MovingAverage<uint16_t> humidityAverage   { 20 };
};
