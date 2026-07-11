/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sht4x_configuration.h"

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>

#include <cstdio>
#include <cstdint>

LOG_MODULE_DECLARE(sensor, CONFIG_SENSOR_LOG_LEVEL);

namespace
{
#ifdef CONFIG_DT_HAS_SENSIRION_SHT4X_ENABLED
const i2c_dt_spec sSht4xI2c =
	I2C_DT_SPEC_GET(DT_COMPAT_GET_ANY_STATUS_OKAY(sensirion_sht4x));
#endif
} /* namespace */

std::optional<std::string> ReadSht4xSerialNumber()
{
#ifdef CONFIG_DT_HAS_SENSIRION_SHT4X_ENABLED
	static constexpr uint8_t kReadSerialNumberCommand = 0x89;
	uint8_t rx[6] = {};
	int rc = i2c_write_dt(&sSht4xI2c, &kReadSerialNumberCommand, sizeof(kReadSerialNumberCommand));
	if (rc != 0) {
		LOG_WRN("Failed to send SHT4x read serial number command: %d", rc);
		return std::nullopt;
	}

	k_msleep(1);

	rc = i2c_read_dt(&sSht4xI2c, rx, sizeof(rx));
	if (rc != 0) {
		LOG_WRN("Failed to read SHT4x serial number: %d", rc);
		return std::nullopt;
	}
	const uint32_t serial = (static_cast<uint32_t>(rx[0]) << 24) |
	                        (static_cast<uint32_t>(rx[1]) << 16) |
	                        (static_cast<uint32_t>(rx[3]) << 8)  |
	                         static_cast<uint32_t>(rx[4]);
	char buf[11];
	std::snprintf(buf, sizeof(buf), "0x%08X", serial);
	return std::string{ buf };
#else
	return std::nullopt;
#endif
}
