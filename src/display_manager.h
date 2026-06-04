/*
 * SPDX-License-Identifier: LicenseRef-Apache-2.0
 */

#pragma once

#ifdef CONFIG_DISPLAY

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <lvgl.h>

#include <optional>

class DisplayManager {
public:
	static DisplayManager &Instance()
	{
		static DisplayManager sInstance;
		return sInstance;
	}

	void Init();
	void UpdateMeasurements(std::optional<int16_t>  temperatureHundredths,
	                        std::optional<uint16_t> humidityHundredths);
	void UpdateSignalStrength(bool connected, uint8_t lqi);
	void SetDecontaminationStatus(bool active, uint32_t elapsedSeconds);
	void SetSensorInfo(const char *secondaryName,
	                   std::optional<uint16_t> secondaryHumidityHundredths);
	void RefreshDisplay();

private:
	/// Submitted to the IoWorker queue by Init(); thunks to InitOnWorker().
	static void InitHandler(k_work *);

	/// Runs the LVGL/SSD16XX bring-up on the IoWorker thread.
	void InitOnWorker();

	/// Custom LVGL draw callback for the signal-strength widget.
	static void SignalDrawCallback(lv_event_t *event);

	/// Builds one sensor card; returns the value label so the caller can
	/// update it later.
	lv_obj_t *CreateSensorCard(lv_obj_t *parent, const char *title,
	                            const char *unit);
	/// Creates a Phosphor glyph label on `screen`, vertically centred on
	/// `valueLabel`. Must be called after `lv_obj_update_layout()` so that
	/// `valueLabel`'s coordinates are valid.
	void CreateIcon(lv_obj_t *screen, uint32_t codepoint, lv_obj_t *valueLabel);
	void DrawMeasurements();
	void DrawSignalBars();
	void DrawDecontamination();
	void DrawSensorInfo();

	enum class State : uint8_t {
		Ready         = 0, // 0 is chosen for Ready so the hot-path test in RefreshDisplay compiles to cbz/cbnz on Cortex-M.
		Uninitialised = 1,
		Unavailable   = 2,
	};

	static constexpr uint8_t kFullUpdateInterval = 200;

	const struct device *mDev   = nullptr;
	State                mState = State::Uninitialised;
	k_work               mInitWork{};
	uint8_t              mPartialUpdateCount = 0;

	std::optional<int16_t>  mCurrentTemperature;
	std::optional<uint16_t> mCurrentHumidity;
	bool        mCurrentConnected                     = false;
	uint8_t     mCurrentLqi                           = 0;
	bool        mCurrentDecontaminationActive         = false;
	uint32_t    mCurrentDecontaminationElapsedSeconds = 0;
	const char *mCurrentSecondarySensorName           = nullptr;
	std::optional<uint16_t> mCurrentSecondaryHumidity;

	std::optional<int16_t>  mLastTemperature;
	std::optional<uint16_t> mLastHumidity;
	bool        mLastConnected                     = false;
	uint8_t     mLastLqi                           = UINT8_MAX;
	bool        mLastDecontaminationActive         = false;
	uint32_t    mLastDecontaminationElapsedSeconds = UINT32_MAX;
	const char *mLastSecondarySensorName           = nullptr;
	std::optional<uint16_t> mLastSecondaryHumidity;

	lv_obj_t *mValueTemperature      = nullptr;
	lv_obj_t *mValueHumidity         = nullptr;
	lv_obj_t *mSignalWidget          = nullptr;
	lv_obj_t *mDecontaminationLabel  = nullptr;
	lv_obj_t *mSecondaryDeltaLabel   = nullptr;
};

#endif /* CONFIG_DISPLAY */
