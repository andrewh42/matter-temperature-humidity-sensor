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

	static constexpr uint32_t kMeasurementsInitialMs = 5'000;
	static constexpr uint32_t kMeasurementsIntervalMs = 60'000;

	CHIP_ERROR ConfigureMeasurementValidityRanges();
	int16_t mTemperatureMeasurementAttributeMinValue = 0;
	int16_t mTemperatureMeasurementAttributeMaxValue = 0;
	uint16_t mHumidityMeasurementAttributeMinValue = 0;
	uint16_t mHumidityMeasurementAttributeMaxValue = 0;

	MovingAverage<int16_t>  mTemperatureMovingAverage{ 20 }; // alpha = 20/32 = 0.625
	MovingAverage<uint16_t> mHumidityMovingAverage{ 20 };

	tl::expected<std::tuple<int16_t, uint16_t>, int> ReadSensor();
	void UpdateTemperatureClusterState(int16_t temperatureHundredths);
	void UpdateRelativeHumidityClusterState(uint16_t humidityHundredths);

	static void MeasurementsTimerHandler();

#ifdef CONFIG_DISPLAY
	std::tuple<bool, uint8_t> GetThreadConnectivity();
#endif

	static void ButtonEventHandler(Nrf::ButtonState state, Nrf::ButtonMask hasChanged);

	static void LEDStateHandler();
};
