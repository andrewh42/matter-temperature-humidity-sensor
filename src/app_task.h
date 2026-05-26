/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "board/board.h"
#include "moving_average.h"

#include <platform/CHIPDeviceLayer.h>
#include <tuple>
#include <tl/expected.hpp>

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
	k_timer sDecontaminationTimer;

	static constexpr uint32_t kMeasurementsInitialMs = 5'000;
	static constexpr uint32_t kMeasurementsIntervalMs = 60'000;

	static constexpr uint32_t kDecontaminationIntervalMs             = 2'000;
	static constexpr uint32_t kDecontaminationLedOnMs                = 50;
	static constexpr uint32_t kDecontaminationMaxDurationMs          = 5 * 60 * 1000;
	static constexpr uint16_t kDecontaminationHumidityExitHundredths = 100; // 1.00%
	static constexpr int32_t  kDecontaminationHeaterLevel            = 14;  // 100% of maximum
	static constexpr int32_t  kHeaterLevelOff                        = 0;

	bool    mDecontaminationActive        = false;
	int64_t mDecontaminationStartUptimeMs = 0;

	bool    mCalibrationRequested        = false;

	CHIP_ERROR ConfigureMeasurementValidityRanges();
	int16_t mTemperatureMeasurementAttributeMinValue = 0;
	int16_t mTemperatureMeasurementAttributeMaxValue = 0;
	uint16_t mHumidityMeasurementAttributeMinValue = 0;
	uint16_t mHumidityMeasurementAttributeMaxValue = 0;

	MovingAverage<int16_t>  mHdc302xTemperatureMovingAverage{ 20 }; // alpha = 20/32 = 0.625
	MovingAverage<uint16_t> mHdc302xHumidityMovingAverage   { 20 };
	MovingAverage<int16_t>  mSht4xTemperatureMovingAverage  { 20 };
	MovingAverage<uint16_t> mSht4xHumidityMovingAverage     { 20 };

	tl::expected<std::tuple<int16_t, uint16_t>, int> ReadSensor(const device *dev, const char *name);
	void UpdateTemperatureClusterState(int16_t temperatureHundredths);
	void UpdateRelativeHumidityClusterState(uint16_t humidityHundredths);

	void ToggleActiveSensor();

	void RequestHumidityCalibration();
	void WriteHumidityCalibrationOffset(uint16_t sht4xSmoothedHundredths,
	                                    uint16_t hdc302xSmoothedHundredths);

	void StartDecontamination();
	void StopDecontamination();
	void RunDecontaminationCycle();

	static void MeasurementsTimerHandler();
	static void DecontaminationTimerHandler();

#ifdef CONFIG_DISPLAY
	std::tuple<bool, uint8_t> GetThreadConnectivity();
#endif

	static void ButtonEventHandler(Nrf::ButtonState state, Nrf::ButtonMask hasChanged);

	static void LEDStateHandler();
};
