/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "board/board.h"

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

	void UpdateClustersState();

private:
	CHIP_ERROR Init();
	k_timer sMeasurementsTimer;

	static constexpr uint16_t kMeasurementsIntervalMs = 30'000;

	CHIP_ERROR ConfigureMeasurementValidityRanges();
	int16_t mTemperatureMeasurementAttributeMinValue = 0;
	int16_t mTemperatureMeasurementAttributeMaxValue = 0;
	uint16_t mHumidityMeasurementAttributeMinValue = 0;
	uint16_t mHumidityMeasurementAttributeMaxValue = 0;

	void UpdateTemperatureClusterState();
	void UpdateRelativeHumidityClusterState();

	static void MeasurementsTimerHandler();

	static void ButtonEventHandler(Nrf::ButtonState state, Nrf::ButtonMask hasChanged);

	static void LEDStateHandler();
};
