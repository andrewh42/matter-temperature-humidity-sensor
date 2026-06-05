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

	/// Identifier shown for this device's primary sensor on the status bar.
	/// Stored by pointer; @p name must remain valid for the lifetime of
	/// the call. Pass nullptr to hide the primary-name label.
	void SetPrimarySensorName(const char *name);

	/// Records whether a paired secondary sensor is configured. Intended
	/// to be called once during startup. Determines whether the primary
	/// name label uses the single-sensor display countdown or remains
	/// visible indefinitely.
	void SetSecondarySensorPresent(bool present);

	/// Latest reading from the paired secondary sensor, in hundredths of
	/// %RH. Drives the offset-from-secondary status item. Pass
	/// std::nullopt when no fresh reading is available.
	void SetSecondaryHumidity(std::optional<uint16_t> humidityHundredths);

	/// Updates the displayed HDC302x humidity calibration offset
	/// (hundredths of %RH, signed). Pass std::nullopt to hide -- e.g.
	/// when the primary sensor is not an HDC302x.
	void SetHumidityCalibrationOffset(std::optional<int16_t> offsetHundredths);

	void RefreshDisplay();

private:
	/// Submitted to the IoWorker queue by Init(); thunks to InitOnWorker().
	static void InitHandler(k_work *);

	/// Orchestrates display bring-up on the IoWorker thread: device init
	/// followed by UI construction, transitioning mState accordingly.
	void InitOnWorker();

	/// Acquires the display device and confirms it is ready. Returns true
	/// on success; the caller transitions mState.
	bool InitDisplayDevice();

	/// Builds the LVGL widget tree on the active screen. Precondition:
	/// the display device is ready.
	void BuildUserInterface();

	/// Builds the top header bar and its children (signal widget,
	/// decontamination label, status-bar container with sensor labels).
	void BuildStatusBar(lv_obj_t *screen);

	/// Builds the main sensor container (temperature card, divider,
	/// humidity card).
	void BuildMeasurementsContainer(lv_obj_t *screen);

	/// Places the Phosphor icons next to the sensor value labels. Must be
	/// called after a layout pass so the value label coordinates are valid.
	void PlaceFloatingIcons(lv_obj_t *screen);

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

	/// In single-sensor mode, the primary name and HDC302x humidity
	/// calibration offset display for this many refreshes after boot,
	/// then hide. Tunable here without a pristine rebuild.
	static constexpr uint8_t kSingleSensorInformationDisplayCount = 10;

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
	const char *mCurrentPrimarySensorName             = nullptr;
	bool        mSecondarySensorPresent               = false;
	std::optional<uint16_t> mCurrentSecondaryHumidity;
	std::optional<int16_t>  mCurrentHumidityCalibrationOffset;
	uint8_t     mTemporarySensorInfoRefreshesRemaining       = kSingleSensorInformationDisplayCount;

	std::optional<int16_t>  mLastTemperature;
	std::optional<uint16_t> mLastHumidity;
	bool        mLastConnected                     = false;
	uint8_t     mLastLqi                           = UINT8_MAX;
	bool        mLastDecontaminationActive         = false;
	uint32_t    mLastDecontaminationElapsedSeconds = UINT32_MAX;
	const char *mLastPrimarySensorName             = nullptr;
	std::optional<uint16_t> mLastSecondaryHumidity;
	std::optional<int16_t>  mLastHumidityCalibrationOffset;
	bool        mLastPrimaryNameVisible            = false;
	bool        mLastCalibrationOffsetVisible      = false;

	lv_obj_t *mValueTemperature            = nullptr;
	lv_obj_t *mValueHumidity               = nullptr;
	lv_obj_t *mSignalWidget                = nullptr;
	lv_obj_t *mDecontaminationLabel        = nullptr;
	lv_obj_t *mStatusBarContainer          = nullptr;
	lv_obj_t *mPrimarySensorLabel          = nullptr;
	lv_obj_t *mOffsetFromSecondaryLabel    = nullptr;
	lv_obj_t *mCalibrationOffsetContainer  = nullptr;
	lv_obj_t *mCalibrationOffsetGlyph      = nullptr;
	lv_obj_t *mCalibrationOffsetValue      = nullptr;
};

#endif /* CONFIG_DISPLAY */
