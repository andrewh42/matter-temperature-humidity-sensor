/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "board/board.h"
#ifdef CONFIG_APP_HDC302X_MAINTENANCE_FEATURES
#include "hdc302x_decontamination_controller.h"
#include "hdc302x_humidity_calibrator.h"
#endif
#include "matter_reporter.h"
#include "sensor.h"

#include <platform/CHIPDeviceLayer.h>


struct Identify;

class AppTask {
public:
	static AppTask &Instance()
	{
		static AppTask sAppTask;
		return sAppTask;
	};

	CHIP_ERROR StartApp();

	void UpdateMeasurements();

private:
	CHIP_ERROR Init();
	k_timer sMeasurementsTimer;

	static constexpr uint32_t kMeasurementsInitialMs = 5'000;
	static constexpr uint32_t kMeasurementsIntervalMs = 60'000;

#ifdef CONFIG_APP_HDC302X_MAINTENANCE_FEATURES
	HDC302xDecontaminationController mDecontaminationController;
	void HandleDecontaminationButton();

	bool    mCalibrationRequested        = false;
#endif

	MatterReporter mMatterReporter;

	Sensor  mHdc302xSensor;
	Sensor  mSht4xSensor;
	Sensor *mPrimarySensor   = nullptr;
	Sensor *mSecondarySensor = nullptr;

	void TogglePrimarySensor();

#ifdef CONFIG_APP_HDC302X_MAINTENANCE_FEATURES
	void RequestHumidityCalibration();
	HDC302xHumidityCalibrator mHumidityCalibrator;
#endif

	static void MeasurementsTimerHandler();

	static void ButtonEventHandler(Nrf::ButtonState state, Nrf::ButtonMask hasChanged);

	static void LEDStateHandler();
};
