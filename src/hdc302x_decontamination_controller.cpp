/*
 * SPDX-License-Identifier: LicenseRef-Apache-2.0
 */

#include "hdc302x_decontamination_controller.h"

#include "app/task_executor.h"
#include "board/board.h"

#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/ti_hdc302x.h>
#include <zephyr/logging/log.h>

#ifdef CONFIG_DISPLAY
#include "display_manager.h"
#endif

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

namespace
{
constexpr int16_t  kTemperatureInvalidSentinel = 0x8000;
constexpr uint16_t kHumidityInvalidSentinel    = 0xffff;
} /* namespace */

void HDC302xDecontaminationController::Init(const device *hdc302xDevice,
                                            Callback onStarted,
                                            Callback onStopped,
                                            void *callbackContext)
{
	mDevice          = hdc302xDevice;
	mOnStarted       = onStarted;
	mOnStopped       = onStopped;
	mCallbackContext = callbackContext;

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

void HDC302xDecontaminationController::Start()
{
	if (mActive) {
		return;
	}
	if (mDevice == nullptr) {
		LOG_WRN("Decontamination requires HDC302x device");
		return;
	}

	if (mOnStarted != nullptr) {
		mOnStarted(mCallbackContext);
	}

	SetHeater(kHeaterLevel);

	mStartUptimeMs = k_uptime_get();
	mActive        = true;

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

	if (mOnStopped != nullptr) {
		mOnStopped(mCallbackContext);
	}

	LOG_INF("Decontamination stopped");
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

	struct sensor_value temperature;
	int16_t temperatureHundredths = kTemperatureInvalidSentinel;
	if (sensor_channel_get(mDevice, SENSOR_CHAN_AMBIENT_TEMP, &temperature) == 0) {
		temperatureHundredths =
			static_cast<int16_t>(temperature.val1 * 100 + temperature.val2 / 10000);
	}

	struct sensor_value humidity;
	uint16_t humidityHundredths = kHumidityInvalidSentinel;
	if (sensor_channel_get(mDevice, SENSOR_CHAN_HUMIDITY, &humidity) == 0) {
		humidityHundredths =
			static_cast<uint16_t>(humidity.val1 * 100 + humidity.val2 / 10000);
	}

	const int64_t elapsedMs = k_uptime_get() - mStartUptimeMs;

	LOG_DBG("Decon t=%lld.%03llds  T=%d.%02dC  RH=%u.%02u%%",
	        elapsedMs / 1000, elapsedMs % 1000,
	        temperatureHundredths / 100, temperatureHundredths % 100,
	        humidityHundredths / 100, humidityHundredths % 100);

#ifdef CONFIG_DISPLAY
	DisplayManager::Instance().UpdateMeasurements(temperatureHundredths, humidityHundredths);
	DisplayManager::Instance().SetDecontaminationStatus(
		true, static_cast<uint32_t>(elapsedMs / 1000));
	DisplayManager::Instance().RefreshDisplay();
#endif

	const bool humidityValid = (humidityHundredths != kHumidityInvalidSentinel);
	if (elapsedMs >= kMaxDurationMs ||
	    (humidityValid && humidityHundredths < kHumidityExitHundredths)) {
		Stop();
	}
}
