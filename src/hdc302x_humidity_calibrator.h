/*
 * SPDX-License-Identifier: LicenseRef-Apache-2.0
 */

#pragma once

#include <cstdint>
#include <optional>

struct device;

/// Programs the HDC302x's persistent humidity offset (in the sensor's offset
/// EEPROM) so its readings agree with a trusted reference reading -- in this
/// project, a smoothed SHT4x sample. The HDC302x must be temporarily switched
/// into manual-measurement mode before the EEPROM can be written; that mode
/// transition is handled internally via RAII so callers cannot leave the
/// sensor stuck in manual mode by skipping the restore step.
class HDC302xHumidityCalibrator {
public:
	HDC302xHumidityCalibrator() = default;

	void Init(const device *hdc302xDevice) { mDevice = hdc302xDevice; }

	/// Returns true iff the offset was actually updated. Callers can use the
	/// return value to know when to flush dependent state (e.g. a moving
	/// average of the sensor's readings).
	bool Apply(std::optional<uint16_t> referenceHundredths,
	           std::optional<uint16_t> targetHundredths);

private:
	// Driver accepts ±127 * 0.1953125 % ≈ ±24.8 % RH. Reject values that
	// would otherwise be silently clamped by the driver.
	static constexpr int32_t kMaxAbsOffsetHundredths = 2480;

	const device *mDevice = nullptr;
};
