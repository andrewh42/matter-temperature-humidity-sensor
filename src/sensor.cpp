/*
 * SPDX-License-Identifier: LicenseRef-Apache-2.0
 */

#include "sensor.h"

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

Sensor::Sensor(const device *device, const char *sensorName)
	: name(sensorName)
{
	if (device == nullptr) {
		return;
	}
	if (!device_is_ready(device)) {
		LOG_WRN("%s sensor device not ready; ignoring", sensorName);
		return;
	}
	dev = device;
}

tl::expected<Sensor::Readings, int> Sensor::Read()
{
	const int result = sensor_sample_fetch(dev);
	if (result != 0) {
		LOG_ERR("Fetching data from %s sensor failed with: %d", name, result);
		return tl::unexpected(result);
	}

	Readings readings;

	struct sensor_value sTemperature;
	const int temp_result = sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &sTemperature);
	if (temp_result == 0) {
		LOG_DBG("New %s temperature measurement %d.%06d C", name, sTemperature.val1, sTemperature.val2);
		const int16_t temperatureHundredths =
			static_cast<int16_t>(sTemperature.val1 * 100 + sTemperature.val2 / 10000);
		readings.temperature = temperatureAverage.update(temperatureHundredths);
	} else {
		LOG_ERR("Getting temperature measurement data from %s failed with: %d", name, temp_result);
	}

	struct sensor_value sHumidity;
	const int humidity_result = sensor_channel_get(dev, SENSOR_CHAN_HUMIDITY, &sHumidity);
	if (humidity_result == 0) {
		LOG_DBG("New %s relative humidity measurement %d.%06d%%", name, sHumidity.val1, sHumidity.val2);
		const uint16_t humidityHundredths =
			static_cast<uint16_t>(sHumidity.val1 * 100 + sHumidity.val2 / 10000);
		readings.humidity = humidityAverage.update(humidityHundredths);
	} else {
		LOG_ERR("Getting humidity measurement data from %s failed with: %d", name, humidity_result);
	}

	return readings;
}
