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

#include <app-common/zap-generated/attributes/Accessors.h>

#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/ti_hdc302x.h>
#include <zephyr/drivers/sensor/sht4x.h>
#include <zephyr/logging/log.h>

#ifdef CONFIG_DISPLAY
#include "display_manager.h"
#include <openthread/link.h>
#include <openthread/thread.h>
#endif

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

using namespace ::chip;
using namespace ::chip::app;
using namespace ::chip::DeviceLayer;

namespace
{
constexpr chip::EndpointId kTemperatureSensorEndpointId = 1;
constexpr chip::EndpointId kHumiditySensorEndpointId = 2;

constexpr int16_t kTemperatureMeasurementAttributeInvalidValue = 0x8000;
constexpr int16_t kHumidityMeasurementAttributeInvalidValue = 0xffff;

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
const device *sActiveSensorDev    = nullptr;
char const   *sActiveSensorName   = nullptr;
const device *sInactiveSensorDev  = nullptr;
char const   *sInactiveSensorName = nullptr;

Nrf::Matter::IdentifyCluster sIdentifyTemperatureCluster(kTemperatureSensorEndpointId);
Nrf::Matter::IdentifyCluster sIdentifyHumidityCluster(kHumiditySensorEndpointId);

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

	if ((DK_BTN4_MSK & state & hasChanged)) {
		Nrf::PostTask([] {
			if (Instance().mDecontaminationActive) {
				Instance().StopDecontamination();
			} else {
				Instance().StartDecontamination();
			}
		});
	}

	if ((DK_BTN2_MSK & state & hasChanged)) {
		Nrf::PostTask([] { Instance().ToggleActiveSensor(); });
	}

	if ((DK_BTN1_MSK & state & hasChanged)) {
		Nrf::PostTask([] { Instance().RequestHumidityCalibration(); });
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

void AppTask::DecontaminationTimerHandler()
{
	Instance().RunDecontaminationCycle();
}

#ifdef CONFIG_DISPLAY
std::tuple<bool, uint8_t> AppTask::GetThreadConnectivity()
{
	bool connected = false;
	uint8_t lqi = 0;
	ThreadStackMgr().LockThreadStack();
	otInstance *ot = ThreadStackMgrImpl().OTInstance();
	const otDeviceRole role = otThreadGetDeviceRole(ot);
	connected = (role != OT_DEVICE_ROLE_DISABLED && role != OT_DEVICE_ROLE_DETACHED);
	if (role == OT_DEVICE_ROLE_CHILD) {
		int8_t rssi = 0;
		if (otThreadGetParentAverageRssi(ot, &rssi) == OT_ERROR_NONE) {
			lqi = otLinkConvertRssToLinkQuality(ot, rssi);
		}
	} else if (IS_ENABLED(CONFIG_OPENTHREAD_FTD) &&
		   (role == OT_DEVICE_ROLE_ROUTER || role == OT_DEVICE_ROLE_LEADER)) {
		otNeighborInfoIterator iter = OT_NEIGHBOR_INFO_ITERATOR_INIT;
		otNeighborInfo info;
		while (otThreadGetNextNeighborInfo(ot, &iter, &info) == OT_ERROR_NONE) {
			if (info.mLinkQualityIn > lqi) {
				lqi = info.mLinkQualityIn;
			}
		}
	}
	ThreadStackMgr().UnlockThreadStack();
	return {connected, lqi};
}
#endif

void AppTask::UpdateTemperatureClusterState(int16_t newValue)
{
	if (newValue > mTemperatureMeasurementAttributeMaxValue ||
		newValue < mTemperatureMeasurementAttributeMinValue) {
		/* Read value exceeds permitted limits, so assign invalid value code to it. */
		newValue = kTemperatureMeasurementAttributeInvalidValue;
	}

	Protocols::InteractionModel::Status status = Clusters::TemperatureMeasurement::Attributes::MeasuredValue::Set(
		kTemperatureSensorEndpointId, newValue);
	if (status != Protocols::InteractionModel::Status::Success) {
		LOG_ERR("Updating temperature measurement %x", to_underlying(status));
	}
}

void AppTask::UpdateRelativeHumidityClusterState(uint16_t newValue)
{
	if (newValue > mHumidityMeasurementAttributeMaxValue ||
		newValue < mHumidityMeasurementAttributeMinValue) {
		/* Read value exceeds permitted limits, so assign invalid value code to it. */
		newValue = kHumidityMeasurementAttributeInvalidValue;
	}

	Protocols::InteractionModel::Status status = Clusters::RelativeHumidityMeasurement::Attributes::MeasuredValue::Set(
		kHumiditySensorEndpointId, newValue);
	if (status != Protocols::InteractionModel::Status::Success) {
		LOG_ERR("Updating relative humidity measurement %x", to_underlying(status));
	}
}

tl::expected<std::tuple<int16_t, uint16_t>, int> AppTask::ReadSensor(const device *dev, const char *name)
{
	const int result = sensor_sample_fetch(dev);
	if (result != 0) {
		Nrf::LEDWidget &sampleFailedLED = Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED4);
		sampleFailedLED.Set(true);
		LOG_ERR("Fetching data from %s sensor failed with: %d", name, result);
		sampleFailedLED.Set(false);
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
		temperatureHundredths = kTemperatureMeasurementAttributeInvalidValue;
	}

	struct sensor_value sHumidity;
	uint16_t humidityHundredths;
	const int humidity_result = sensor_channel_get(dev, SENSOR_CHAN_HUMIDITY, &sHumidity);
	if (humidity_result == 0) {
		LOG_DBG("New %s relative humidity measurement %d.%06d%%", name, sHumidity.val1, sHumidity.val2);
		humidityHundredths = static_cast<int16_t>(sHumidity.val1 * 100 + sHumidity.val2 / 10000);
	} else {
		LOG_ERR("Getting humidity measurement data from %s failed with: %d", name, humidity_result);
		humidityHundredths = kHumidityMeasurementAttributeInvalidValue;
	}

	return tl::expected<std::tuple<int16_t, uint16_t>, int>({temperatureHundredths, humidityHundredths});
}

void AppTask::UpdateMeasurements()
{
	if (mDecontaminationActive) {
		return;
	}

#ifndef CONFIG_DISPLAY
	Nrf::LEDWidget &clusterUpdateLED = Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED3);
	clusterUpdateLED.Set(true);
#endif

	auto smoothActive = [this](const device *dev, int16_t rawTemp, uint16_t rawHum) {
		int16_t  smoothedTemp = rawTemp;
		uint16_t smoothedHum  = rawHum;
		if (dev == sHdc302xSensorDev) {
			if (rawTemp != kTemperatureMeasurementAttributeInvalidValue) {
				smoothedTemp = mHdc302xTemperatureMovingAverage.update(rawTemp);
			}
			if (rawHum != static_cast<uint16_t>(kHumidityMeasurementAttributeInvalidValue)) {
				smoothedHum = mHdc302xHumidityMovingAverage.update(rawHum);
			}
		} else {
			if (rawTemp != kTemperatureMeasurementAttributeInvalidValue) {
				smoothedTemp = mSht4xTemperatureMovingAverage.update(rawTemp);
			}
			if (rawHum != static_cast<uint16_t>(kHumidityMeasurementAttributeInvalidValue)) {
				smoothedHum = mSht4xHumidityMovingAverage.update(rawHum);
			}
		}
		return std::make_tuple(smoothedTemp, smoothedHum);
	};

	auto activeResult = ReadSensor(sActiveSensorDev, sActiveSensorName);
	if (!activeResult) {
		LOG_ERR("Failed to read active sensor data: %d", activeResult.error());
#ifndef CONFIG_DISPLAY
		clusterUpdateLED.Set(false);
#endif
		return;
	}
	auto [rawActiveTemp, rawActiveHum] = activeResult.value();
	auto [activeTemp, activeHum] = smoothActive(sActiveSensorDev, rawActiveTemp, rawActiveHum);

	uint16_t inactiveHum = static_cast<uint16_t>(kHumidityMeasurementAttributeInvalidValue);
	if (sInactiveSensorDev != nullptr) {
		auto inactiveResult = ReadSensor(sInactiveSensorDev, sInactiveSensorName);
		if (inactiveResult) {
			auto [rawInactiveTemp, rawInactiveHum] = inactiveResult.value();
			auto [unusedInactiveTemp, smoothedInactiveHum] =
				smoothActive(sInactiveSensorDev, rawInactiveTemp, rawInactiveHum);
			(void)unusedInactiveTemp;
			inactiveHum = smoothedInactiveHum;
		} else {
			LOG_WRN("Failed to read inactive sensor data: %d", inactiveResult.error());
		}
	}

	if (mCalibrationRequested) {
		mCalibrationRequested = false;
		const bool sht4xActive      = (sActiveSensorDev == sSht4xSensorDev);
		const bool hdc302xAvailable = (sHdc302xSensorDev != nullptr);
		const bool readingsValid =
			activeHum   != static_cast<uint16_t>(kHumidityMeasurementAttributeInvalidValue) &&
			inactiveHum != static_cast<uint16_t>(kHumidityMeasurementAttributeInvalidValue);
		if (!sht4xActive || !hdc302xAvailable) {
			LOG_WRN("Humidity calibration requires SHT4x active and HDC302x present; skipped");
		} else if (!readingsValid) {
			LOG_WRN("Humidity calibration skipped: smoothed reading unavailable");
		} else {
			// activeHum = SHT4x smoothed; inactiveHum = HDC302x smoothed (since SHT4x is active).
			WriteHumidityCalibrationOffset(activeHum, inactiveHum);
		}
	}

	UpdateTemperatureClusterState(activeTemp);
	UpdateRelativeHumidityClusterState(activeHum);

#ifdef CONFIG_DISPLAY
	auto [connected, lqi] = GetThreadConnectivity();
	LOG_DBG("Thread connectivity: %s, LQI: %d", connected ? "connected" : "not connected", lqi);
	DisplayManager::Instance().UpdateSignalStrength(connected, lqi);

	DisplayManager::Instance().UpdateMeasurements(activeTemp, activeHum);
	DisplayManager::Instance().SetSensorInfo(sInactiveSensorName, inactiveHum);
	DisplayManager::Instance().RefreshDisplay();
#else
	clusterUpdateLED.Set(false);
#endif
}

void AppTask::StartDecontamination()
{
	if (mDecontaminationActive) {
		return;
	}

	if (sActiveSensorDev != sHdc302xSensorDev || sHdc302xSensorDev == nullptr) {
		LOG_WRN("Decontamination requires HDC302x as active sensor");
		return;
	}

	k_timer_stop(&sMeasurementsTimer);

	const struct sensor_value heaterLevel = {.val1 = kDecontaminationHeaterLevel, .val2 = 0};
	const int heaterResult = sensor_attr_set(sHdc302xSensorDev, SENSOR_CHAN_ALL,
	                                         (enum sensor_attribute)SENSOR_ATTR_HEATER_LEVEL,
	                                         &heaterLevel);
	if (heaterResult != 0) {
		LOG_ERR("Failed to enable HDC302x heater: %d", heaterResult);
	}

	mDecontaminationStartUptimeMs = k_uptime_get();
	mDecontaminationActive        = true;

	k_timer_start(&sDecontaminationTimer, K_MSEC(kDecontaminationIntervalMs),
	              K_MSEC(kDecontaminationIntervalMs));

	Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED4).Blink(kDecontaminationLedOnMs,
	                                                   kDecontaminationIntervalMs - kDecontaminationLedOnMs);

#ifdef CONFIG_DISPLAY
	DisplayManager::Instance().SetDecontaminationStatus(true, 0);
	DisplayManager::Instance().RefreshDisplay();
#endif

	LOG_INF("Decontamination started (heater level %d)", kDecontaminationHeaterLevel);
}

void AppTask::StopDecontamination()
{
	if (!mDecontaminationActive) {
		return;
	}

	k_timer_stop(&sDecontaminationTimer);

	if (sHdc302xSensorDev != nullptr) {
		const struct sensor_value heaterLevel = {.val1 = kHeaterLevelOff, .val2 = 0};
		const int heaterResult = sensor_attr_set(sHdc302xSensorDev, SENSOR_CHAN_ALL,
		                                         (enum sensor_attribute)SENSOR_ATTR_HEATER_LEVEL,
		                                         &heaterLevel);
		if (heaterResult != 0) {
			LOG_ERR("Failed to disable HDC302x heater: %d", heaterResult);
		}
	}

	mDecontaminationActive = false;

	Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED4).Set(false);

#ifdef CONFIG_DISPLAY
	DisplayManager::Instance().SetDecontaminationStatus(false, 0);
	DisplayManager::Instance().RefreshDisplay();
#endif

	k_timer_start(&sMeasurementsTimer, K_MSEC(kMeasurementsIntervalMs),
	              K_MSEC(kMeasurementsIntervalMs));

	LOG_INF("Decontamination stopped");
}

void AppTask::RunDecontaminationCycle()
{
	if (!mDecontaminationActive) {
		return;
	}

	auto sensorResult = ReadSensor(sHdc302xSensorDev, "HDC302x");
	if (!sensorResult) {
		LOG_ERR("Decon: failed to read sensor: %d", sensorResult.error());
		return;
	}

	auto [temperatureHundredths, humidityHundredths] = sensorResult.value();
	const int64_t elapsedMs = k_uptime_get() - mDecontaminationStartUptimeMs;

	LOG_DBG("Decon t=%lld.%03llds  T=%d.%02dC  RH=%u.%02u%%",
	        elapsedMs / 1000, elapsedMs % 1000,
	        temperatureHundredths / 100, temperatureHundredths % 100,
	        humidityHundredths / 100, humidityHundredths % 100);

#ifdef CONFIG_DISPLAY
	DisplayManager::Instance().UpdateMeasurements(temperatureHundredths, humidityHundredths);
	DisplayManager::Instance().SetDecontaminationStatus(true, static_cast<uint32_t>(elapsedMs / 1000));
	DisplayManager::Instance().RefreshDisplay();
#endif

	const bool humidityValid =
		humidityHundredths != static_cast<uint16_t>(kHumidityMeasurementAttributeInvalidValue);
	if (elapsedMs >= kDecontaminationMaxDurationMs ||
	    (humidityValid && humidityHundredths < kDecontaminationHumidityExitHundredths)) {
		StopDecontamination();
	}
}

void AppTask::ToggleActiveSensor()
{
	if (mDecontaminationActive) {
		LOG_WRN("Sensor toggle ignored during decontamination");
		return;
	}
	if (sInactiveSensorDev == nullptr) {
		LOG_WRN("Sensor toggle requires both sensors in device tree");
		return;
	}

	const device *previousActiveDev    = sActiveSensorDev;
	const char   *previousActiveName   = sActiveSensorName;
	sActiveSensorDev    = sInactiveSensorDev;
	sActiveSensorName   = sInactiveSensorName;
	sInactiveSensorDev  = previousActiveDev;
	sInactiveSensorName = previousActiveName;

	LOG_INF("Active sensor: %s -> %s", sInactiveSensorName, sActiveSensorName);

	UpdateMeasurements();
}

void AppTask::RequestHumidityCalibration()
{
	mCalibrationRequested = true;
	LOG_INF("Humidity calibration requested (will apply on next measurement tick)");
}

void AppTask::WriteHumidityCalibrationOffset(uint16_t sht4xSmoothedHundredths,
                                             uint16_t hdc302xSmoothedHundredths)
{
	// Driver accepts ±127 * 0.1953125 % ≈ ±24.8 % RH. Reject values that would be clamped.
	constexpr int32_t kMaxAbsOffsetHundredths = 2480;

	const struct sensor_value manual = {.val1 = HDC302X_SENSOR_MEAS_INTERVAL_MANUAL, .val2 = 0};
	int rc = sensor_attr_set(sHdc302xSensorDev, SENSOR_CHAN_ALL,
	                         (enum sensor_attribute)SENSOR_ATTR_INTEGRATION_TIME, &manual);
	if (rc != 0) {
		LOG_ERR("Calibration: failed to enter HDC302x manual mode (%d)", rc);
		return;
	}

	auto restoreAutoMode = [] {
		const struct sensor_value autoMeas = {.val1 = HDC302X_SENSOR_MEAS_INTERVAL_0_5, .val2 = 0};
		const int restoreRc = sensor_attr_set(sHdc302xSensorDev, SENSOR_CHAN_ALL,
		                                      (enum sensor_attribute)SENSOR_ATTR_INTEGRATION_TIME,
		                                      &autoMeas);
		if (restoreRc != 0) {
			LOG_ERR("Calibration: failed to re-enable HDC302x auto-measurement (%d) -- sensor stuck in manual mode",
			        restoreRc);
		}
	};

	struct sensor_value existing = {};
	rc = sensor_attr_get(sHdc302xSensorDev, SENSOR_CHAN_HUMIDITY, SENSOR_ATTR_OFFSET, &existing);
	if (rc != 0) {
		LOG_ERR("Calibration: failed to read existing HDC302x offset (%d)", rc);
		restoreAutoMode();
		return;
	}

	const int32_t existingHundredths = existing.val1 * 100 + existing.val2 / 10000;
	const int32_t deltaHundredths    = static_cast<int32_t>(sht4xSmoothedHundredths)
	                                 - static_cast<int32_t>(hdc302xSmoothedHundredths);
	const int32_t newHundredths      = existingHundredths + deltaHundredths;

	if (newHundredths > kMaxAbsOffsetHundredths || newHundredths < -kMaxAbsOffsetHundredths) {
		LOG_WRN("Calibration: new offset %d.%02d%% out of HDC302x range (+/-24.80%%); skipped",
		        newHundredths / 100,
		        (newHundredths % 100 < 0) ? -(newHundredths % 100) : (newHundredths % 100));
		restoreAutoMode();
		return;
	}

	const struct sensor_value next = {.val1 = newHundredths / 100,
	                                  .val2 = (newHundredths % 100) * 10000};
	rc = sensor_attr_set(sHdc302xSensorDev, SENSOR_CHAN_HUMIDITY, SENSOR_ATTR_OFFSET, &next);
	if (rc != 0) {
		LOG_ERR("Calibration: failed to program HDC302x offset (%d)", rc);
		restoreAutoMode();
		return;
	}

	auto absMod = [](int32_t v) { return (v % 100 < 0) ? -(v % 100) : (v % 100); };
	LOG_INF("HDC302x humidity offset programmed: existing=%d.%02d%%, delta=%d.%02d%%, new=%d.%02d%%",
	        existingHundredths / 100, absMod(existingHundredths),
	        deltaHundredths    / 100, absMod(deltaHundredths),
	        newHundredths      / 100, absMod(newHundredths));

	mHdc302xHumidityMovingAverage.reset();

	restoreAutoMode();
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

	if (sHdc302xSensorDev != nullptr && !device_is_ready(sHdc302xSensorDev)) {
		LOG_ERR("HDC302x sensor device not ready");
		return chip::System::MapErrorZephyr(-ENODEV);
	}
	if (sSht4xSensorDev != nullptr && !device_is_ready(sSht4xSensorDev)) {
		LOG_ERR("SHT4x sensor device not ready");
		return chip::System::MapErrorZephyr(-ENODEV);
	}

	if (sHdc302xSensorDev != nullptr) {
		sActiveSensorDev    = sHdc302xSensorDev;
		sActiveSensorName   = "HDC302x";
		sInactiveSensorDev  = sSht4xSensorDev;
		sInactiveSensorName = (sSht4xSensorDev != nullptr) ? "SHT4x" : nullptr;
	} else {
		sActiveSensorDev    = sSht4xSensorDev;
		sActiveSensorName   = "SHT4x";
		sInactiveSensorDev  = nullptr;
		sInactiveSensorName = nullptr;
	}

	if (sHdc302xSensorDev != nullptr) {
		const struct sensor_value integration_time = {.val1 = (int32_t)HDC302X_SENSOR_MEAS_INTERVAL_0_5, .val2 = 0};
		sensor_attr_set(sHdc302xSensorDev, SENSOR_CHAN_ALL, (enum sensor_attribute)SENSOR_ATTR_INTEGRATION_TIME, &integration_time); // 0.5 Hz
	}

#ifdef CONFIG_DISPLAY
	ReturnErrorOnFailure(DisplayManager::Instance().Init());
#endif

	return Nrf::Matter::StartServer();
}

CHIP_ERROR AppTask::ConfigureMeasurementValidityRanges()
{
	DataModel::Nullable<int16_t> val;
	Protocols::InteractionModel::Status status =
		Clusters::TemperatureMeasurement::Attributes::MinMeasuredValue::Get(kTemperatureSensorEndpointId, val);
	if (status != Protocols::InteractionModel::Status::Success || val.IsNull()) {
		LOG_ERR("Failed to get temperature measurement min value %x", to_underlying(status));
		return CHIP_ERROR_INCORRECT_STATE;
	}
	mTemperatureMeasurementAttributeMinValue = val.Value();

	status = Clusters::TemperatureMeasurement::Attributes::MaxMeasuredValue::Get(kTemperatureSensorEndpointId, val);
	if (status != Protocols::InteractionModel::Status::Success || val.IsNull()) {
		LOG_ERR("Failed to get temperature measurement max value %x", to_underlying(status));
		return CHIP_ERROR_INCORRECT_STATE;
	}
	mTemperatureMeasurementAttributeMaxValue = val.Value();

	DataModel::Nullable<uint16_t> uval;
	status = Clusters::RelativeHumidityMeasurement::Attributes::MinMeasuredValue::Get(kHumiditySensorEndpointId, uval);
	if (status != Protocols::InteractionModel::Status::Success || uval.IsNull()) {
		LOG_ERR("Failed to get humidity measurement min value %x", to_underlying(status));
		return CHIP_ERROR_INCORRECT_STATE;
	}
	mHumidityMeasurementAttributeMinValue = uval.Value();

	status = Clusters::RelativeHumidityMeasurement::Attributes::MaxMeasuredValue::Get(kHumiditySensorEndpointId, uval);
	if (status != Protocols::InteractionModel::Status::Success || uval.IsNull()) {
		LOG_ERR("Failed to get humidity measurement max value %x", to_underlying(status));
		return CHIP_ERROR_INCORRECT_STATE;
	}
	mHumidityMeasurementAttributeMaxValue = uval.Value();

	return CHIP_NO_ERROR;
}

CHIP_ERROR AppTask::StartApp()
{
	ReturnErrorOnFailure(Init());
	ReturnErrorOnFailure(ConfigureMeasurementValidityRanges());

	k_timer_init(
		&sMeasurementsTimer, [](k_timer *) { Nrf::PostTask([] { MeasurementsTimerHandler(); }); }, nullptr);
	k_timer_start(&sMeasurementsTimer, K_MSEC(kMeasurementsInitialMs), K_MSEC(kMeasurementsIntervalMs));

	k_timer_init(
		&sDecontaminationTimer, [](k_timer *) { Nrf::PostTask([] { DecontaminationTimerHandler(); }); }, nullptr);

	while (true) {
		Nrf::DispatchNextTask();
	}

	return CHIP_NO_ERROR;
}
