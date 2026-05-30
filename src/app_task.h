/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "board/board.h"
#include "hdc302x_decontamination_controller.h"
#include "hdc302x_humidity_calibrator.h"
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

	HDC302xDecontaminationController mDecontaminationController;
	void HandleDecontaminationButton();
	void OnDecontaminationStarted();
	void OnDecontaminationStopped();
	static void DecontaminationStartedCallback(void *context);
	static void DecontaminationStoppedCallback(void *context);

	bool    mCalibrationRequested        = false;

	MatterReporter mMatterReporter;

	Sensor  mHdc302xSensor;
	Sensor  mSht4xSensor;
	Sensor *mPrimarySensor   = nullptr;
	Sensor *mSecondarySensor = nullptr;

	void TogglePrimarySensor();

	void RequestHumidityCalibration();
	HDC302xHumidityCalibrator mHumidityCalibrator;

	CHIP_ERROR ConfigureHdc302xDefaults();

	static void MeasurementsTimerHandler();

	static void ButtonEventHandler(Nrf::ButtonState state, Nrf::ButtonMask hasChanged);

	static void LEDStateHandler();
};
