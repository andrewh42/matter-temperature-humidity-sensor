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
#include "io_worker.h"
#include "measurement_worker.h"
#include "lib/core/CHIPError.h"
#include "system/SystemError.h"

#include <zephyr/logging/log.h>

#include <optional>

#ifdef CONFIG_DISPLAY
#include "display_manager.h"
#endif

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

using namespace ::chip;
using namespace ::chip::app;
using namespace ::chip::DeviceLayer;

namespace
{
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
		MeasurementWorker::Instance().RequestTogglePrimarySensor();
	}

#ifdef CONFIG_APP_HDC302X_MAINTENANCE_FEATURES
	if ((DK_BTN1_MSK & state & hasChanged)) {
		MeasurementWorker::Instance().RequestHumidityCalibration();
	}

	if ((DK_BTN4_MSK & state & hasChanged)) {
		MeasurementWorker::Instance().RequestToggleDecontamination();
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

	if (int err = IoWorker::Instance().Init(); err) {
		return chip::System::MapErrorZephyr(err);
	}

#ifdef CONFIG_DISPLAY
	/* DisplayManager::Init() submits the SSD16XX/LVGL bring-up to the
	 * IoWorker, so the worker must already exist. FIFO ordering of the
	 * IoWorker queue places this work ahead of the first measurement tick
	 * armed in MeasurementWorker::Start(). */
	DisplayManager::Instance().Init();
#endif

	ReturnErrorOnFailure(Nrf::Matter::StartServer());

	ReturnErrorOnFailure(mMatterReporter.LoadValidityRanges());

	if (int err = MeasurementWorker::Instance().Init(
		[](std::optional<int16_t> t, std::optional<uint16_t> h) {
			Nrf::PostTask([t, h] { Instance().mMatterReporter.Publish(t, h); });
		}); err) {
		return chip::System::MapErrorZephyr(err);
	}

	return CHIP_NO_ERROR;
}

CHIP_ERROR AppTask::StartApp()
{
	ReturnErrorOnFailure(Init());

	MeasurementWorker::Instance().Start();

	while (true) {
		Nrf::DispatchNextTask();
	}

	return CHIP_NO_ERROR;
}
