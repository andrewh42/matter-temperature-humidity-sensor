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

const device *sHdc302xSensorDev = DEVICE_DT_GET_ONE(ti_hdc302x);

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

tl::expected<std::tuple<int16_t, uint16_t>, int> AppTask::ReadSensor()
{
	const int result = sensor_sample_fetch(sHdc302xSensorDev);
	if (result != 0) {
		Nrf::LEDWidget &sampleFailedLED = Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED4);
		sampleFailedLED.Set(true);
		LOG_ERR("Fetching data from HDC302x sensor failed with: %d", result);
		sampleFailedLED.Set(false);
		return tl::unexpected(result);
	}

	struct sensor_value sTemperature;
	int16_t temperatureHundredths;
	const int temp_result = sensor_channel_get(sHdc302xSensorDev, SENSOR_CHAN_AMBIENT_TEMP, &sTemperature);
	if (temp_result == 0) {
		LOG_DBG("New HDC302x temperature measurement %d.%06d C", sTemperature.val1, sTemperature.val2);
		temperatureHundredths = static_cast<int16_t>(sTemperature.val1 * 100 + sTemperature.val2 / 10000);
	} else {
		LOG_ERR("Getting temperature measurement data from HDC302x failed with: %d", temp_result);
		temperatureHundredths = kTemperatureMeasurementAttributeInvalidValue;
	}

	struct sensor_value sHumidity;
	uint16_t humidityHundredths;
	const int humidity_result = sensor_channel_get(sHdc302xSensorDev, SENSOR_CHAN_HUMIDITY, &sHumidity);
	if (humidity_result == 0) {
		LOG_DBG("New HDC302x relative humidity measurement %d.%06d%%", sHumidity.val1, sHumidity.val2);
		humidityHundredths = static_cast<int16_t>(sHumidity.val1 * 100 + sHumidity.val2 / 10000);
	} else {
		LOG_ERR("Getting humidity measurement data from HDC302x failed with: %d", humidity_result);
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
	auto sensorResult = ReadSensor();
	if (!sensorResult) {
		LOG_ERR("Failed to read sensor data: %d", sensorResult.error());
#ifndef CONFIG_DISPLAY
		clusterUpdateLED.Set(false);
#endif
		return;
	}

	auto [rawTemperatureHundredths, rawHumidityHundredths] = sensorResult.value();

	int16_t temperatureHundredths = rawTemperatureHundredths;
	if (rawTemperatureHundredths != kTemperatureMeasurementAttributeInvalidValue) {
		temperatureHundredths = mTemperatureMovingAverage.update(rawTemperatureHundredths);
		LOG_DBG("Temperature: %d.%02d C (raw)", rawTemperatureHundredths / 100, rawTemperatureHundredths % 100);
		LOG_DBG("Temperature: %d.%02d C (smoothed)", temperatureHundredths / 100, temperatureHundredths % 100);
	}

	uint16_t humidityHundredths = rawHumidityHundredths;
	if (rawHumidityHundredths != static_cast<uint16_t>(kHumidityMeasurementAttributeInvalidValue)) {
		humidityHundredths = mHumidityMovingAverage.update(rawHumidityHundredths);
		LOG_DBG("Humidity: %d.%02d%% (raw)", rawHumidityHundredths / 100, rawHumidityHundredths % 100);
		LOG_DBG("Humidity: %d.%02d%% (smoothed)", humidityHundredths / 100, humidityHundredths % 100);
	}

	UpdateTemperatureClusterState(temperatureHundredths);
	UpdateRelativeHumidityClusterState(humidityHundredths);

#ifdef CONFIG_DISPLAY
	auto [connected, lqi] = GetThreadConnectivity();
	LOG_DBG("Thread connectivity: %s, LQI: %d", connected ? "connected" : "not connected", lqi);
	DisplayManager::Instance().UpdateSignalStrength(connected, lqi);

	DisplayManager::Instance().UpdateMeasurements(temperatureHundredths, humidityHundredths);
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

	const struct sensor_value heaterLevel = {.val1 = kHeaterLevelOff, .val2 = 0};
	const int heaterResult = sensor_attr_set(sHdc302xSensorDev, SENSOR_CHAN_ALL,
	                                         (enum sensor_attribute)SENSOR_ATTR_HEATER_LEVEL,
	                                         &heaterLevel);
	if (heaterResult != 0) {
		LOG_ERR("Failed to disable HDC302x heater: %d", heaterResult);
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

	auto sensorResult = ReadSensor();
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

	if (!device_is_ready(sHdc302xSensorDev)) {
		LOG_ERR("HDC302x sensor device not ready");
		return chip::System::MapErrorZephyr(-ENODEV);
	}

	const struct sensor_value integration_time = {.val1 = (int32_t)HDC302X_SENSOR_MEAS_INTERVAL_0_5, .val2 = 0};
	sensor_attr_set(sHdc302xSensorDev, SENSOR_CHAN_ALL, (enum sensor_attribute)SENSOR_ATTR_INTEGRATION_TIME, &integration_time); // 0.5 Hz

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
