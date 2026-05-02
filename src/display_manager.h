/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#ifdef CONFIG_DISPLAY

#include <lib/core/CHIPError.h>
#include <zephyr/device.h>

class DisplayManager {
public:
	static DisplayManager &Instance()
	{
		static DisplayManager sInstance;
		return sInstance;
	}

	CHIP_ERROR Init();
	void UpdateMeasurements(int16_t temperatureHundredths, uint16_t humidityHundredths);

private:
	static constexpr uint8_t kFullUpdateInterval = 30;

	const struct device *mDev = nullptr;
	bool mInitialized = false;
	int16_t mLastTemperature = INT16_MIN;
	uint16_t mLastHumidity = UINT16_MAX;
	uint8_t mPartialUpdateCount = 0;
};

#endif /* CONFIG_DISPLAY */
