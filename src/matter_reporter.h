/*
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <lib/core/CHIPError.h>
#include <platform/CHIPDeviceLayer.h>

#include <cstdint>
#include <optional>

class MatterReporter {
public:
	static constexpr chip::EndpointId kTemperatureEndpointId = 1;
	static constexpr chip::EndpointId kHumidityEndpointId    = 2;

	/// Reads the four min/max bounds from the Matter clusters.
	/// Must be called after the Matter server is running.
	CHIP_ERROR LoadValidityRanges();

	/// Writes both measurements to their clusters, substituting the
	/// invalid-value sentinel when a reading is empty or falls outside the
	/// loaded bounds.
	void Publish(std::optional<int16_t>  temperatureHundredths,
	             std::optional<uint16_t> humidityHundredths);

private:
	int16_t  mTemperatureMin = 0;
	int16_t  mTemperatureMax = 0;
	uint16_t mHumidityMin    = 0;
	uint16_t mHumidityMax    = 0;
};
