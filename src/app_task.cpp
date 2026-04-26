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
#include "../drivers/sensor/ti_hdc302x/ti_hdc302x.h"
#include <zephyr/logging/log.h>

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
	Instance().UpdateClustersState();
}

void AppTask::UpdateTemperatureClusterState()
{
	struct sensor_value sTemperature;
	int result = sensor_channel_get(sHdc302xSensorDev, SENSOR_CHAN_AMBIENT_TEMP, &sTemperature);
	if (result == 0) {
		/* Defined by cluster temperature measured value = 100 x temperature in degC with resolution of
		 * 0.01 degC. val1 is an integer part of the value and val2 is fractional part in one-millionth
		 * parts. To achieve resolution of 0.01 degC val2 needs to be divided by 10000. */
		int16_t newValue = static_cast<int16_t>(sTemperature.val1 * 100 + sTemperature.val2 / 10000);

		if (newValue > mTemperatureMeasurementAttributeMaxValue ||
		    newValue < mTemperatureMeasurementAttributeMinValue) {
			/* Read value exceeds permitted limits, so assign invalid value code to it. */
			newValue = kTemperatureMeasurementAttributeInvalidValue;
		}
		LOG_DBG("New HDC302x temperature measurement %d.%0d C", sTemperature.val1, sTemperature.val2);

		Protocols::InteractionModel::Status status = Clusters::TemperatureMeasurement::Attributes::MeasuredValue::Set(
			kTemperatureSensorEndpointId, newValue);
		if (status != Protocols::InteractionModel::Status::Success) {
			LOG_ERR("Updating temperature measurement %x", to_underlying(status));
		}
	} else {
		LOG_ERR("Getting temperature measurement data from HDC302x failed with: %d", result);
	}
}

void AppTask::UpdateRelativeHumidityClusterState()
{
	struct sensor_value sHumidity;
	int result = sensor_channel_get(sHdc302xSensorDev, SENSOR_CHAN_HUMIDITY, &sHumidity);
	if (result == 0) {
		/* Defined by cluster humidity measured value = 100 x humidity in %RH with resolution of 0.01 %.
		 * val1 is an integer part of the value and val2 is fractional part in one-millionth parts.
		 * To achieve resolution of 0.01 % val2 needs to be divided by 10000. */
		uint16_t newValue = static_cast<int16_t>(sHumidity.val1 * 100 + sHumidity.val2 / 10000);

		if (newValue > mHumidityMeasurementAttributeMaxValue ||
		    newValue < mHumidityMeasurementAttributeMinValue) {
			/* Read value exceeds permitted limits, so assign invalid value code to it. */
			newValue = kHumidityMeasurementAttributeInvalidValue;
		}
		LOG_DBG("New HDC302x relative humidity measurement %d.%0d %%", sHumidity.val1, sHumidity.val2);

		Protocols::InteractionModel::Status status = Clusters::RelativeHumidityMeasurement::Attributes::MeasuredValue::Set(
			kHumiditySensorEndpointId, newValue);
		if (status != Protocols::InteractionModel::Status::Success) {
			LOG_ERR("Updating relative humidity measurement %x", to_underlying(status));
		}
	} else {
		LOG_ERR("Getting humidity measurement data from HDC302x failed with: %d", result);
	}
}

void AppTask::UpdateClustersState()
{
	Nrf::LEDWidget &clusterUpdateLED = Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED3);
	clusterUpdateLED.Set(true);
	const int result = sensor_sample_fetch(sHdc302xSensorDev);

	if (result == 0) {
		UpdateTemperatureClusterState();
		UpdateRelativeHumidityClusterState();
		clusterUpdateLED.Set(false);
	} else {
		Nrf::LEDWidget &sampleFailedLED = Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED4);
		sampleFailedLED.Set(true);
		LOG_ERR("Fetching data from HDC302x sensor failed with: %d", result);
		sampleFailedLED.Set(false);
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
	k_timer_start(&sMeasurementsTimer, K_MSEC(kMeasurementsIntervalMs), K_MSEC(kMeasurementsIntervalMs));

	while (true) {
		Nrf::DispatchNextTask();
	}

	return CHIP_NO_ERROR;
}
