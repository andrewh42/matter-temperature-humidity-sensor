/*
 * SPDX-License-Identifier: LicenseRef-Apache-2.0
 */

#include "hdc302x_decontamination_controller.h"

#include "app/task_executor.h"
#include "board/board.h"

#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/ti_hdc302x.h>
#include <zephyr/logging/log.h>

#include <cstdio>
#include <optional>

#ifdef CONFIG_DISPLAY
#include "display_manager.h"
#endif

LOG_MODULE_DECLARE(sensor, CONFIG_SENSOR_LOG_LEVEL);

void HDC302xDecontaminationController::Init(const device *hdc302xDevice)
{
	mDevice = hdc302xDevice;

	k_timer_init(
		&mTimer,
		[](k_timer *timer) {
			auto *self =
				static_cast<HDC302xDecontaminationController *>(k_timer_user_data_get(timer));
			Nrf::PostTask([self] { self->RunCycle(); });
		},
		nullptr);
	k_timer_user_data_set(&mTimer, this);
}

void HDC302xDecontaminationController::Toggle()
{
	if (mActive) {
		Stop();
	} else {
		Start();
	}
}

void HDC302xDecontaminationController::SetHeater(int32_t level)
{
	if (mDevice == nullptr) {
		return;
	}
	const struct sensor_value heaterLevel = {.val1 = level, .val2 = 0};
	const int result = sensor_attr_set(mDevice, SENSOR_CHAN_ALL,
	                                   (enum sensor_attribute)SENSOR_ATTR_HEATER_LEVEL,
	                                   &heaterLevel);
	if (result != 0) {
		LOG_ERR("Failed to %s HDC302x heater: %d",
		        (level == kHeaterOff) ? "disable" : "enable", result);
	}
}

const char *HDC302xDecontaminationController::ToString(StopReason reason)
{
	switch (reason) {
	case StopReason::Toggle:                   return "toggle";
	case StopReason::TimerExpired:             return "timer expired";
	case StopReason::HumidityThresholdReached: return "humidity below threshold";
	}
	return "unknown";
}

void HDC302xDecontaminationController::Start()
{
	if (mActive) {
		return;
	}
	if (mDevice == nullptr) {
		LOG_WRN("Decontamination requires HDC302x device");
		return;
	}

	SetHeater(kHeaterLevel);

	mStartUptimeMs          = k_uptime_get();
	mCycleCount             = 0;
	mLastHumidityHundredths = std::nullopt;
	mActive                 = true;

	k_timer_start(&mTimer, K_MSEC(kIntervalMs), K_MSEC(kIntervalMs));

	Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED4).Blink(kLedOnMs, kIntervalMs - kLedOnMs);

#ifdef CONFIG_DISPLAY
	DisplayManager::Instance().SetDecontaminationStatus(true, 0);
	DisplayManager::Instance().RefreshDisplay();
#endif

	LOG_INF("Decontamination started (heater level %d)", kHeaterLevel);
}

void HDC302xDecontaminationController::Stop()
{
	Stop(StopReason::Toggle);
}

void HDC302xDecontaminationController::Stop(StopReason reason)
{
	if (!mActive) {
		return;
	}

	k_timer_stop(&mTimer);

	SetHeater(kHeaterOff);

	mActive = false;

	Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED4).Set(false);

#ifdef CONFIG_DISPLAY
	DisplayManager::Instance().SetDecontaminationStatus(false, 0);
	DisplayManager::Instance().RefreshDisplay();
#endif

	char buffer[16];
	const char *humidityText;
	if (mLastHumidityHundredths) {
		snprintf(buffer, sizeof(buffer), "%u.%02u%%",
		         *mLastHumidityHundredths / 100, *mLastHumidityHundredths % 100);
		humidityText = buffer;
	} else {
		humidityText = "n/a";
	}
	LOG_INF("Decontamination stopped (final RH %s, reason: %s)",
	        humidityText, ToString(reason));
}

void HDC302xDecontaminationController::RunCycle()
{
	if (!mActive) {
		return;
	}

	const int fetchResult = sensor_sample_fetch(mDevice);
	if (fetchResult != 0) {
		LOG_ERR("Decon: failed to fetch sensor: %d", fetchResult);
		return;
	}

	std::optional<int16_t> temperatureHundredths;
	struct sensor_value temperature;
	if (sensor_channel_get(mDevice, SENSOR_CHAN_AMBIENT_TEMP, &temperature) == 0) {
		temperatureHundredths =
			static_cast<int16_t>(temperature.val1 * 100 + temperature.val2 / 10000);
	}

	std::optional<uint16_t> humidityHundredths;
	struct sensor_value humidity;
	if (sensor_channel_get(mDevice, SENSOR_CHAN_HUMIDITY, &humidity) == 0) {
		humidityHundredths =
			static_cast<uint16_t>(humidity.val1 * 100 + humidity.val2 / 10000);
	}
	mLastHumidityHundredths = humidityHundredths;

	const int64_t elapsedMs = k_uptime_get() - mStartUptimeMs;

	const int16_t  loggedTemperature = temperatureHundredths.value_or(0);
	const uint16_t loggedHumidity    = humidityHundredths.value_or(0);
	LOG_DBG("Decon t=%lld.%03llds  T=%d.%02dC  RH=%u.%02u%%",
	        elapsedMs / 1000, elapsedMs % 1000,
	        loggedTemperature / 100, loggedTemperature % 100,
	        loggedHumidity / 100, loggedHumidity % 100);

#ifdef CONFIG_DISPLAY
	if (mCycleCount % kDisplayUpdateInterval == 0) {
		DisplayManager::Instance().UpdateMeasurements(temperatureHundredths, humidityHundredths);
		DisplayManager::Instance().SetDecontaminationStatus(
			true, static_cast<uint32_t>(elapsedMs / 1000));
		DisplayManager::Instance().RefreshDisplay();
	}
#endif
	++mCycleCount;

	if (humidityHundredths && *humidityHundredths < kHumidityExitHundredths) {
		Stop(StopReason::HumidityThresholdReached);
	} else if (elapsedMs >= kMaxDurationMs) {
		Stop(StopReason::TimerExpired);
	}
}
