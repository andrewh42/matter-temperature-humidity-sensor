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
#include "lib/core/CHIPError.h"

#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/ti_hdc302x.h>
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

	if ((DK_BTN1_MSK & state & hasChanged)) {
		Nrf::PostTask([] { Instance().RequestHumidityCalibration(); });
	}

	if ((DK_BTN2_MSK & state & hasChanged)) {
		Nrf::PostTask([] { Instance().TogglePrimarySensor(); });
	}

	if ((DK_BTN4_MSK & state & hasChanged)) {
		Nrf::PostTask([] { Instance().HandleDecontaminationButton(); });
	}
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

void AppTask::DecontaminationStartedCallback(void *context)
{
	static_cast<AppTask *>(context)->OnDecontaminationStarted();
}

void AppTask::DecontaminationStoppedCallback(void *context)
{
	static_cast<AppTask *>(context)->OnDecontaminationStopped();
}

void AppTask::OnDecontaminationStarted()
{
	k_timer_stop(&sMeasurementsTimer);
}

void AppTask::OnDecontaminationStopped()
{
	k_timer_start(&sMeasurementsTimer, K_MSEC(kMeasurementsIntervalMs),
	              K_MSEC(kMeasurementsIntervalMs));
}

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

void AppTask::UpdateMeasurements()
{
	if (mDecontaminationController.Active()) {
		return;
	}

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
	if (mDecontaminationController.Active()) {
		LOG_WRN("Sensor toggle ignored during decontamination");
		return;
	}
	if (mSecondarySensor == nullptr) {
		LOG_WRN("Sensor toggle requires both sensors in device tree");
		return;
	}

	std::swap(mPrimarySensor, mSecondarySensor);

	LOG_INF("Primary sensor: %s -> %s", mSecondarySensor->name, mPrimarySensor->name);

	UpdateMeasurements();
}

void AppTask::RequestHumidityCalibration()
{
	mCalibrationRequested = true;
	LOG_INF("Humidity calibration requested (will apply on next measurement tick)");
}

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

	if (sHdc302xSensorDev == nullptr && sSht4xSensorDev == nullptr) {
		LOG_ERR("No supported sensor found in device tree");
		return chip::System::MapErrorZephyr(-ENODEV);
	}

	const device *hdc302xDev = sHdc302xSensorDev;
	const device *sht4xDev   = sSht4xSensorDev;

	if (hdc302xDev != nullptr && !device_is_ready(hdc302xDev)) {
		LOG_WRN("HDC302x sensor device not ready; ignoring");
		hdc302xDev = nullptr;
	}
	if (sht4xDev != nullptr && !device_is_ready(sht4xDev)) {
		LOG_WRN("SHT4x sensor device not ready; ignoring");
		sht4xDev = nullptr;
	}

	if (hdc302xDev == nullptr && sht4xDev == nullptr) {
		LOG_ERR("No sensor device ready");
		return chip::System::MapErrorZephyr(-ENODEV);
	}

	mHdc302xSensor.dev  = hdc302xDev;
	mHdc302xSensor.name = "HDC302x";
	mSht4xSensor.dev    = sht4xDev;
	mSht4xSensor.name   = "SHT4x";

	if (hdc302xDev != nullptr) {
		mPrimarySensor   = &mHdc302xSensor;
		mSecondarySensor = (sht4xDev != nullptr) ? &mSht4xSensor : nullptr;
	} else {
		mPrimarySensor   = &mSht4xSensor;
		mSecondarySensor = nullptr;
	}

	ReturnErrorOnFailure(ConfigureHdc302xDefaults());

	mHumidityCalibrator.Init(mHdc302xSensor.dev);

#ifdef CONFIG_DISPLAY
	ReturnErrorOnFailure(DisplayManager::Instance().Init());
#endif

	return Nrf::Matter::StartServer();
}

CHIP_ERROR AppTask::ConfigureHdc302xDefaults()
{
	if (!mHdc302xSensor.IsAvailable()) {
		return CHIP_NO_ERROR;
	}

	const struct sensor_value integrationTime = {
		.val1 = static_cast<int32_t>(HDC302X_SENSOR_MEAS_INTERVAL_0_5), .val2 = 0};
	const int result = sensor_attr_set(mHdc302xSensor.dev, SENSOR_CHAN_ALL,
	                                   (enum sensor_attribute)SENSOR_ATTR_INTEGRATION_TIME,
	                                   &integrationTime); // 0.5 Hz
	if (result != 0) {
		LOG_ERR("Failed to set HDC302x integration time: %d", result);
		return chip::System::MapErrorZephyr(result);
	}
	return CHIP_NO_ERROR;
}

CHIP_ERROR AppTask::StartApp()
{
	ReturnErrorOnFailure(Init());
	ReturnErrorOnFailure(mMatterReporter.LoadValidityRanges());

	k_timer_init(
		&sMeasurementsTimer, [](k_timer *) { Nrf::PostTask([] { MeasurementsTimerHandler(); }); }, nullptr);
	k_timer_start(&sMeasurementsTimer, K_MSEC(kMeasurementsInitialMs), K_MSEC(kMeasurementsIntervalMs));

	mDecontaminationController.Init(mHdc302xSensor.dev,
	                                &AppTask::DecontaminationStartedCallback,
	                                &AppTask::DecontaminationStoppedCallback,
	                                this);

	while (true) {
		Nrf::DispatchNextTask();
	}

	return CHIP_NO_ERROR;
}
