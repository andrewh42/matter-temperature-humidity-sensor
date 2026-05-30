/*
 * SPDX-License-Identifier: LicenseRef-Apache-2.0
 */

#include "hdc302x_configuration.h"

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/ti_hdc302x.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

namespace
{
bool SetHdc302xMeasurementInterval(const device *hdc302xDevice, int32_t interval)
{
	const struct sensor_value value = {.val1 = interval, .val2 = 0};
	const int result = sensor_attr_set(hdc302xDevice, SENSOR_CHAN_ALL,
	                                   (enum sensor_attribute)SENSOR_ATTR_INTEGRATION_TIME,
	                                   &value);
	if (result != 0) {
		LOG_ERR("Failed to set HDC302x integration time: %d", result);
		return false;
	}
	return true;
}
} /* namespace */

bool ConfigureHdc302xAutomaticMeasurementMode(const device *hdc302xDevice)
{
	return SetHdc302xMeasurementInterval(hdc302xDevice, HDC302X_SENSOR_MEAS_INTERVAL_0_5);
}

bool ConfigureHdc302xManualMeasurementMode(const device *hdc302xDevice)
{
	return SetHdc302xMeasurementInterval(hdc302xDevice, HDC302X_SENSOR_MEAS_INTERVAL_MANUAL);
}
