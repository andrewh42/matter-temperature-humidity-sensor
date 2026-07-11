/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include "hdc302x_configuration.h"

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/ti_hdc302x.h>
#include <zephyr/logging/log.h>

#include <cstdio>

LOG_MODULE_DECLARE(sensor, CONFIG_SENSOR_LOG_LEVEL);

namespace
{
#ifdef CONFIG_DT_HAS_TI_HDC302X_ENABLED
const i2c_dt_spec sHdc302xI2c =
	I2C_DT_SPEC_GET(DT_COMPAT_GET_ANY_STATUS_OKAY(ti_hdc302x));
#endif

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

std::optional<std::string> ReadHdc302xNistId()
{
#ifdef CONFIG_DT_HAS_TI_HDC302X_ENABLED
	static constexpr uint8_t kCommands[3][2] = {
		{ 0x36, 0x83 }, /* NIST ID bytes 5 and 4 */
		{ 0x36, 0x84 }, /* NIST ID bytes 3 and 2 */
		{ 0x36, 0x85 }, /* NIST ID bytes 1 and 0 */
	};
	uint8_t nist[6] = {};
	for (size_t i = 0; i < 3; ++i) {
		uint8_t rx[3] = {};
		int rc = i2c_write_read_dt(&sHdc302xI2c,
		                           kCommands[i], sizeof(kCommands[i]),
		                           rx, sizeof(rx));
		if (rc != 0) {
			LOG_WRN("Failed to read HDC302x NIST ID: %d", rc);
			return std::nullopt;
		}
		nist[i * 2]     = rx[0];
		nist[i * 2 + 1] = rx[1];
	}
	char buf[15] = {};
	std::snprintf(buf, sizeof(buf), "0x%02X%02X%02X%02X%02X%02X",
	              nist[0], nist[1], nist[2], nist[3], nist[4], nist[5]);
	return std::string{ buf };
#else
	return std::nullopt;
#endif
}
