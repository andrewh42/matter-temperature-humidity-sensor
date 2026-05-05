/*
 * SPDX-License-Identifier: LicenseRef-Apache-2.0
 */

#pragma once

#ifdef CONFIG_DISPLAY

#include <lib/core/CHIPError.h>
#include <zephyr/device.h>
#include <lvgl.h>

class DisplayManager {
public:
	static DisplayManager &Instance()
	{
		static DisplayManager sInstance;
		return sInstance;
	}

	CHIP_ERROR Init();
	void UpdateMeasurements(int16_t temperatureHundredths, uint16_t humidityHundredths);
	void UpdateSignalStrength(bool connected, uint8_t lqi);
	void RefreshDisplay();

private:
	void DrawMeasurements();
	void DrawSignalBars();

	static constexpr uint8_t kFullUpdateInterval = 30;

	const struct device *mDev = nullptr;
	bool mInitialized = false;
	uint8_t mPartialUpdateCount = 0;

	int16_t  mCurrentTemperature = INT16_MIN;
	uint16_t mCurrentHumidity    = UINT16_MAX;
	bool     mCurrentConnected   = false;
	uint8_t  mCurrentLqi         = 0;

	int16_t  mLastTemperature = INT16_MIN;
	uint16_t mLastHumidity    = UINT16_MAX;
	bool     mLastConnected   = false;
	uint8_t  mLastLqi         = UINT8_MAX;

	lv_obj_t *mLabelTemperature  = nullptr;
	lv_obj_t *mLabelHumidity     = nullptr;
	lv_obj_t *mLabelPartial      = nullptr;
	lv_obj_t *mLabelDisconnected = nullptr;
	lv_obj_t *mSignalBars[4]     = {};
};

#endif /* CONFIG_DISPLAY */
