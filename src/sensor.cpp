/*
 * SPDX-License-Identifier: LicenseRef-Apache-2.0
 */

#include "sensor.h"

#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

tl::expected<Sensor::Readings, int> Sensor::Read()
{
	const int result = sensor_sample_fetch(dev);
	if (result != 0) {
		LOG_ERR("Fetching data from %s sensor failed with: %d", name, result);
		return tl::unexpected(result);
	}

	struct sensor_value sTemperature;
	int16_t temperatureHundredths;
	const int temp_result = sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &sTemperature);
	if (temp_result == 0) {
		LOG_DBG("New %s temperature measurement %d.%06d C", name, sTemperature.val1, sTemperature.val2);
		temperatureHundredths = static_cast<int16_t>(sTemperature.val1 * 100 + sTemperature.val2 / 10000);
	} else {
		LOG_ERR("Getting temperature measurement data from %s failed with: %d", name, temp_result);
		temperatureHundredths = kTemperatureInvalid;
	}

	struct sensor_value sHumidity;
	uint16_t humidityHundredths;
	const int humidity_result = sensor_channel_get(dev, SENSOR_CHAN_HUMIDITY, &sHumidity);
	if (humidity_result == 0) {
		LOG_DBG("New %s relative humidity measurement %d.%06d%%", name, sHumidity.val1, sHumidity.val2);
		humidityHundredths = static_cast<uint16_t>(sHumidity.val1 * 100 + sHumidity.val2 / 10000);
	} else {
		LOG_ERR("Getting humidity measurement data from %s failed with: %d", name, humidity_result);
		humidityHundredths = kHumidityInvalid;
	}

	const int16_t smoothedTemperature =
		(temperatureHundredths != kTemperatureInvalid)
			? temperatureAverage.update(temperatureHundredths)
			: temperatureHundredths;
	const uint16_t smoothedHumidity =
		(humidityHundredths != kHumidityInvalid)
			? humidityAverage.update(humidityHundredths)
			: humidityHundredths;

	return Readings{smoothedTemperature, smoothedHumidity};
}
