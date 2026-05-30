/*
 * SPDX-License-Identifier: LicenseRef-Apache-2.0
 */

#include "hdc302x_humidity_calibrator.h"

#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/ti_hdc302x.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

namespace
{
/// RAII guard: switches the HDC302x into manual-measurement mode on construction
/// (the only mode in which the offset EEPROM can be written) and restores the
/// 0.5 Hz auto-measurement cadence on destruction, even when the calibration
/// path takes one of its many early-return branches.
class ManualMeasurementScope {
public:
	explicit ManualMeasurementScope(const device *device) : mDevice(device)
	{
		const struct sensor_value manual = {.val1 = HDC302X_SENSOR_MEAS_INTERVAL_MANUAL,
		                                    .val2 = 0};
		const int result = sensor_attr_set(mDevice, SENSOR_CHAN_ALL,
		                                   (enum sensor_attribute)SENSOR_ATTR_INTEGRATION_TIME,
		                                   &manual);
		if (result != 0) {
			LOG_ERR("Calibration: failed to enter HDC302x manual mode (%d)", result);
			return;
		}
		mEntered = true;
	}

	~ManualMeasurementScope()
	{
		if (!mEntered) {
			return;
		}
		const struct sensor_value autoMeasurement = {.val1 = HDC302X_SENSOR_MEAS_INTERVAL_0_5,
		                                             .val2 = 0};
		const int result = sensor_attr_set(mDevice, SENSOR_CHAN_ALL,
		                                   (enum sensor_attribute)SENSOR_ATTR_INTEGRATION_TIME,
		                                   &autoMeasurement);
		if (result != 0) {
			LOG_ERR("Calibration: failed to re-enable HDC302x auto-measurement (%d) -- sensor stuck in manual mode",
			        result);
		}
	}

	bool Entered() const { return mEntered; }

	ManualMeasurementScope(const ManualMeasurementScope &)            = delete;
	ManualMeasurementScope &operator=(const ManualMeasurementScope &) = delete;

private:
	const device *mDevice;
	bool          mEntered = false;
};

int AbsoluteHundredths(int32_t value)
{
	const int32_t remainder = value % 100;
	return static_cast<int>(remainder < 0 ? -remainder : remainder);
}
} /* namespace */

bool HDC302xHumidityCalibrator::Apply(std::optional<uint16_t> referenceHundredths,
                                      std::optional<uint16_t> targetHundredths)
{
	if (mDevice == nullptr) {
		return false;
	}

	if (!referenceHundredths || !targetHundredths) {
		LOG_WRN("Humidity calibration skipped: readings unavailable");
		return false;
	}

	ManualMeasurementScope scope(mDevice);
	if (!scope.Entered()) {
		return false;
	}

	struct sensor_value existing = {};
	int rc = sensor_attr_get(mDevice, SENSOR_CHAN_HUMIDITY, SENSOR_ATTR_OFFSET, &existing);
	if (rc != 0) {
		LOG_ERR("Calibration: failed to read existing HDC302x offset (%d)", rc);
		return false;
	}

	const int32_t existingHundredths = existing.val1 * 100 + existing.val2 / 10000;
	const int32_t deltaHundredths    = static_cast<int32_t>(*referenceHundredths)
	                                 - static_cast<int32_t>(*targetHundredths);
	const int32_t newHundredths      = existingHundredths + deltaHundredths;

	if (newHundredths > kMaxAbsOffsetHundredths || newHundredths < -kMaxAbsOffsetHundredths) {
		LOG_WRN("Calibration: new offset %d.%02d%% out of HDC302x range (+/-24.80%%); skipped",
		        newHundredths / 100, AbsoluteHundredths(newHundredths));
		return false;
	}

	const struct sensor_value next = {.val1 = newHundredths / 100,
	                                  .val2 = (newHundredths % 100) * 10000};
	rc = sensor_attr_set(mDevice, SENSOR_CHAN_HUMIDITY, SENSOR_ATTR_OFFSET, &next);
	if (rc != 0) {
		LOG_ERR("Calibration: failed to program HDC302x offset (%d)", rc);
		return false;
	}

	LOG_INF("HDC302x humidity offset programmed: existing=%d.%02d%%, delta=%d.%02d%%, new=%d.%02d%%",
	        existingHundredths / 100, AbsoluteHundredths(existingHundredths),
	        deltaHundredths    / 100, AbsoluteHundredths(deltaHundredths),
	        newHundredths      / 100, AbsoluteHundredths(newHundredths));

	return true;
}
