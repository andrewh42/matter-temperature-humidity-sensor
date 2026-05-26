/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "board/board.h"
#include "decontamination_controller.h"
#include "humidity_calibrator.h"
#include "sensor.h"

#include <platform/CHIPDeviceLayer.h>
#include <tuple>

#ifdef CONFIG_DISPLAY
#include <openthread/link.h>
#include <openthread/thread.h>
#endif

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

	DecontaminationController mDecontaminationController;
	void HandleDecontaminationButton();
	void OnDecontaminationStarted();
	void OnDecontaminationStopped();
	static void DecontaminationStartedCallback(void *context);
	static void DecontaminationStoppedCallback(void *context);

	bool    mCalibrationRequested        = false;

	CHIP_ERROR ConfigureMeasurementValidityRanges();
	int16_t mTemperatureMeasurementAttributeMinValue = 0;
	int16_t mTemperatureMeasurementAttributeMaxValue = 0;
	uint16_t mHumidityMeasurementAttributeMinValue = 0;
	uint16_t mHumidityMeasurementAttributeMaxValue = 0;

	Sensor  mHdc302xSensor;
	Sensor  mSht4xSensor;
	Sensor *mPrimarySensor   = nullptr;
	Sensor *mSecondarySensor = nullptr;

	void UpdateTemperatureClusterState(int16_t temperatureHundredths);
	void UpdateRelativeHumidityClusterState(uint16_t humidityHundredths);

	void TogglePrimarySensor();

	void RequestHumidityCalibration();
	HumidityCalibrator mHumidityCalibrator;

	CHIP_ERROR ConfigureHdc302xDefaults();

	static void MeasurementsTimerHandler();

#ifdef CONFIG_DISPLAY
	std::tuple<bool, uint8_t> GetThreadConnectivity();
#endif

	static void ButtonEventHandler(Nrf::ButtonState state, Nrf::ButtonMask hasChanged);

	static void LEDStateHandler();
};
