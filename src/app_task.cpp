/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "app_task.h"

#include "app/matter_init.h"
#include "app/task_executor.h"
#include "board/board.h"
#include "clusters/identify.h"
#include "hdc302x_configuration.h"
#include "lib/core/CHIPError.h"

#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/sht4x.h>
#include <zephyr/logging/log.h>

#ifdef CONFIG_DISPLAY
#include "display_manager.h"
#include "thread_status.h"
#endif

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

using namespace ::chip;
using namespace ::chip::app;
using namespace ::chip::DeviceLayer;

namespace
{
#ifdef CONFIG_DT_HAS_TI_HDC302X_ENABLED
const device *sHdc302xSensorDev = DEVICE_DT_GET_ONE(ti_hdc302x);
#else
const device *sHdc302xSensorDev = nullptr;
#endif
#ifdef CONFIG_DT_HAS_SENSIRION_SHT4X_ENABLED
const device *sSht4xSensorDev = DEVICE_DT_GET_ONE(sensirion_sht4x);
#else
const device *sSht4xSensorDev = nullptr;
#endif

Nrf::Matter::IdentifyCluster sIdentifyTemperatureCluster(MatterReporter::kTemperatureEndpointId);
Nrf::Matter::IdentifyCluster sIdentifyHumidityCluster(MatterReporter::kHumidityEndpointId);

#ifdef CONFIG_CHIP_ICD_UAT_SUPPORT
#define UAT_BUTTON_MASK DK_BTN3_MSK
#endif
} /* namespace */

void AppTask::ButtonEventHandler(Nrf::ButtonState state, Nrf::ButtonMask hasChanged)
{
#ifdef CONFIG_CHIP_ICD_UAT_SUPPORT
	if ((UAT_BUTTON_MASK & state & hasChanged)) {
		LOG_INF("ICD UserActiveMode has been triggered.");
		Server::GetInstance().GetICDManager().OnNetworkActivity();
	}
#endif

	if ((DK_BTN2_MSK & state & hasChanged)) {
		Nrf::PostTask([] { Instance().TogglePrimarySensor(); });
	}

#ifdef CONFIG_APP_HDC302X_MAINTENANCE_FEATURES
	if ((DK_BTN1_MSK & state & hasChanged)) {
		Nrf::PostTask([] { Instance().RequestHumidityCalibration(); });
	}

	if ((DK_BTN4_MSK & state & hasChanged)) {
		Nrf::PostTask([] { Instance().HandleDecontaminationButton(); });
	}
#endif
}

void AppTask::LEDStateHandler()
{
	/* Update the status LED.
	 *
	 * If IPv6 network and service provisioned, the LED is off.
	 *
	 * If the system has BLE connection(s) uptill the stage above, THEN blink the LED at an even
	 * rate of 100ms.
	 *
	 * Otherwise, blink the LED for a very short time. */
	Nrf::LEDWidget &statusLED = Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED1);

	switch (Nrf::GetBoard().GetDeviceState()) {
	case Nrf::DeviceState::DeviceDisconnected:
	case Nrf::DeviceState::DeviceAdvertisingBLE:
		statusLED.Blink(Nrf::LedConsts::StatusLed::Disconnected::kOn_ms,
				      Nrf::LedConsts::StatusLed::Disconnected::kOff_ms);

		break;
	case Nrf::DeviceState::DeviceConnectedBLE:
		statusLED.Blink(Nrf::LedConsts::StatusLed::BleConnected::kOn_ms,
				      Nrf::LedConsts::StatusLed::BleConnected::kOff_ms);
		break;
	case Nrf::DeviceState::DeviceProvisioned:
		statusLED.Set(false);
		break;
	default:
		LOG_ERR("LEDStateHandler: invalid device state");
		break;
	}
}

void AppTask::MeasurementsTimerHandler()
{
	Instance().UpdateMeasurements();
}

#ifdef CONFIG_APP_HDC302X_MAINTENANCE_FEATURES
void AppTask::HandleDecontaminationButton()
{
	if (mDecontaminationController.Active()) {
		mDecontaminationController.Stop();
		return;
	}
	if (mPrimarySensor != &mHdc302xSensor || !mHdc302xSensor.IsAvailable()) {
		LOG_WRN("Decontamination requires HDC302x as primary sensor");
		return;
	}
	mDecontaminationController.Start();
}
#endif

void AppTask::UpdateMeasurements()
{
#ifdef CONFIG_APP_HDC302X_MAINTENANCE_FEATURES
	if (mDecontaminationController.Active()) {
		return;
	}
#endif

#ifndef CONFIG_DISPLAY
	Nrf::LEDWidget &clusterUpdateLED = Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED3);
	clusterUpdateLED.Set(true);
#endif

	auto primaryResult = mPrimarySensor->Read();
	if (!primaryResult) {
		LOG_ERR("Failed to read primary sensor data: %d", primaryResult.error());
#ifndef CONFIG_DISPLAY
		clusterUpdateLED.Set(false);
#endif
		return;
	}
	auto [primaryTemperature, primaryHumidity] = primaryResult.value();

	std::optional<uint16_t> secondaryHumidity;
	if (mSecondarySensor != nullptr) {
		auto secondaryResult = mSecondarySensor->Read();
		if (secondaryResult) {
			secondaryHumidity = secondaryResult.value().humidity;
		} else {
			LOG_WRN("Failed to read secondary sensor data: %d", secondaryResult.error());
		}
	}

#ifdef CONFIG_APP_HDC302X_MAINTENANCE_FEATURES
	if (mCalibrationRequested) {
		mCalibrationRequested = false;
		const bool sht4xPrimary = (mPrimarySensor == &mSht4xSensor);
		if (!sht4xPrimary || !mHdc302xSensor.IsAvailable()) {
			LOG_WRN("Humidity calibration requires SHT4x primary and HDC302x present; skipped");
		} else {
			// primaryHumidity = SHT4x smoothed; secondaryHumidity = HDC302x smoothed (since SHT4x is primary).
			if (mHumidityCalibrator.Apply(primaryHumidity, secondaryHumidity)) {
				mHdc302xSensor.humidityAverage.reset();
			}
		}
	}
#endif

	mMatterReporter.Publish(primaryTemperature, primaryHumidity);

#ifdef CONFIG_DISPLAY
	auto [connected, lqi] = GetThreadConnectivity();
	LOG_DBG("Thread connectivity: %s, LQI: %d", connected ? "connected" : "not connected", lqi);
	DisplayManager::Instance().UpdateSignalStrength(connected, lqi);

	DisplayManager::Instance().UpdateMeasurements(primaryTemperature, primaryHumidity);
	if (mSecondarySensor != nullptr) {
		DisplayManager::Instance().SetSensorInfo(mSecondarySensor->name, secondaryHumidity);
	}
	DisplayManager::Instance().RefreshDisplay();
#else
	clusterUpdateLED.Set(false);
#endif
}

void AppTask::TogglePrimarySensor()
{
#ifdef CONFIG_APP_HDC302X_MAINTENANCE_FEATURES
	if (mDecontaminationController.Active()) {
		LOG_WRN("Sensor toggle ignored during decontamination");
		return;
	}
#endif
	if (mSecondarySensor == nullptr) {
		LOG_WRN("Sensor toggle requires both sensors in device tree");
		return;
	}

	std::swap(mPrimarySensor, mSecondarySensor);

	LOG_INF("Primary sensor: %s -> %s", mSecondarySensor->name, mPrimarySensor->name);

	UpdateMeasurements();
}

#ifdef CONFIG_APP_HDC302X_MAINTENANCE_FEATURES
void AppTask::RequestHumidityCalibration()
{
	mCalibrationRequested = true;
	LOG_INF("Humidity calibration requested (will apply on next measurement tick)");
}
#endif

CHIP_ERROR AppTask::Init()
{
	/* Initialize Matter stack */
	ReturnErrorOnFailure(Nrf::Matter::PrepareServer());

	if (!Nrf::GetBoard().Init(ButtonEventHandler, LEDStateHandler)) {
		LOG_ERR("User interface initialization failed.");
		return CHIP_ERROR_INCORRECT_STATE;
	}

	/* Register Matter event handler that controls the connectivity status LED based on the captured Matter network
	 * state. */
	ReturnErrorOnFailure(Nrf::Matter::RegisterEventHandler(Nrf::Board::DefaultMatterEventHandler, 0));

	ReturnErrorOnFailure(sIdentifyTemperatureCluster.Init());
	ReturnErrorOnFailure(sIdentifyHumidityCluster.Init());

	mHdc302xSensor = Sensor(sHdc302xSensorDev, "HDC302x");
	mSht4xSensor   = Sensor(sSht4xSensorDev,   "SHT4x");

	if (mHdc302xSensor.IsAvailable()) {
		mPrimarySensor   = &mHdc302xSensor;
		mSecondarySensor = mSht4xSensor.IsAvailable() ? &mSht4xSensor : nullptr;

		if (!ConfigureHdc302xAutomaticMeasurementMode(mHdc302xSensor.dev)) {
			return CHIP_ERROR_INTERNAL;
		}
#ifdef CONFIG_APP_HDC302X_MAINTENANCE_FEATURES
		mHumidityCalibrator.Init(mHdc302xSensor.dev);
		mDecontaminationController.Init(mHdc302xSensor.dev);
#endif
	} else if (mSht4xSensor.IsAvailable()) {
		mPrimarySensor   = &mSht4xSensor;
		mSecondarySensor = nullptr;
	} else {
		LOG_ERR("No sensor device available");
		return chip::System::MapErrorZephyr(-ENODEV);
	}

#ifdef CONFIG_DISPLAY
	ReturnErrorOnFailure(DisplayManager::Instance().Init());
#endif

	return Nrf::Matter::StartServer();
}

CHIP_ERROR AppTask::StartApp()
{
	ReturnErrorOnFailure(Init());
	ReturnErrorOnFailure(mMatterReporter.LoadValidityRanges());

	k_timer_init(
		&sMeasurementsTimer, [](k_timer *) { Nrf::PostTask([] { MeasurementsTimerHandler(); }); }, nullptr);
	k_timer_start(&sMeasurementsTimer, K_MSEC(kMeasurementsInitialMs), K_MSEC(kMeasurementsIntervalMs));

	while (true) {
		Nrf::DispatchNextTask();
	}

	return CHIP_NO_ERROR;
}
