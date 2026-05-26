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
	void SetDecontaminationStatus(bool active, uint32_t elapsedSeconds);
	void SetSensorInfo(const char *inactiveName, uint16_t inactiveHumidityHundredths);
	void RefreshDisplay();

private:
	static void SignalDrawCallback(lv_event_t *event);

	lv_obj_t *CreateSensorCard(lv_obj_t *parent, const char *title,
	                            const char *unit);
	void CreateIcon(lv_obj_t *screen, uint32_t codepoint, lv_obj_t *valueLabel);
	void DrawMeasurements();
	void DrawSignalBars();
	void DrawDecontamination();
	void DrawSensorInfo();

	static constexpr uint8_t kFullUpdateInterval = 100;

	const struct device *mDev        = nullptr;
	bool                 mInitialized = false;
	uint8_t              mPartialUpdateCount = 0;

	int16_t     mCurrentTemperature                   = INT16_MIN;
	uint16_t    mCurrentHumidity                      = UINT16_MAX;
	bool        mCurrentConnected                     = false;
	uint8_t     mCurrentLqi                           = 0;
	bool        mCurrentDecontaminationActive         = false;
	uint32_t    mCurrentDecontaminationElapsedSeconds = 0;
	const char *mCurrentInactiveSensorName            = nullptr;
	uint16_t    mCurrentInactiveHumidity              = UINT16_MAX;

	int16_t     mLastTemperature                   = INT16_MIN;
	uint16_t    mLastHumidity                      = UINT16_MAX;
	bool        mLastConnected                     = false;
	uint8_t     mLastLqi                           = UINT8_MAX;
	bool        mLastDecontaminationActive         = false;
	uint32_t    mLastDecontaminationElapsedSeconds = UINT32_MAX;
	const char *mLastInactiveSensorName            = nullptr;
	uint16_t    mLastInactiveHumidity              = UINT16_MAX;

	lv_obj_t *mValueTemperature      = nullptr;
	lv_obj_t *mValueHumidity         = nullptr;
	lv_obj_t *mSignalWidget          = nullptr;
	lv_obj_t *mDecontaminationLabel  = nullptr;
	lv_obj_t *mInactiveDeltaLabel    = nullptr;
};

#endif /* CONFIG_DISPLAY */
