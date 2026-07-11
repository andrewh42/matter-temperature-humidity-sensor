/*
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "sensor.h"

#ifdef CONFIG_APP_HDC302X_MAINTENANCE_FEATURES
#include "hdc302x_decontamination_controller.h"
#include "hdc302x_humidity_calibrator.h"
#endif

#include <zephyr/kernel.h>

#include <cstdint>
#include <functional>
#include <optional>

/// Owns all measurement-tick state (sensors, moving averages, optional
/// HDC302x maintenance controllers) and runs the periodic tick body on the
/// dedicated I/O worker thread. Sensor I2C reads and any subsequent display
/// refreshes therefore do not block the CHIP task.
///
/// Public methods are split by intended caller thread:
///   - Init() and Start() are called from the CHIP task during startup.
///   - The Request* methods are called from the CHIP task (typically from a
///     button event handler hop) and internally submit a k_work item to the
///     I/O worker so that mutation of MeasurementWorker state stays on the
///     worker thread.
class MeasurementWorker {
public:
	static MeasurementWorker &Instance()
	{
		static MeasurementWorker sInstance;
		return sInstance;
	}

	/// Callback invoked from the I/O worker thread once per tick with the
	/// freshly-smoothed primary-sensor readings. The callback is responsible
	/// for any thread hop required to reach the publish target (typically a
	/// Nrf::PostTask hop back to the CHIP task before touching ZAP setters).
	using PublishFn =
		std::function<void(std::optional<int16_t>, std::optional<uint16_t>)>;

	/// Constructs the sensors, selects the primary, programs HDC302x mode,
	/// and stores @p publish for use by Tick(). Call from the CHIP task
	/// before Start(). Returns 0 on success or a negative errno on failure.
	int Init(PublishFn publish);

	/// Starts the periodic k_timer that drives tick submission to the
	/// I/O worker. After this call the tick body can run.
	void Start();

	/// Toggle the primary sensor between HDC302x and SHT4x (when both are
	/// available). Safe to call from any thread; the work happens on the
	/// I/O worker.
	void RequestTogglePrimarySensor();

#ifdef CONFIG_APP_HDC302X_MAINTENANCE_FEATURES
	/// Schedule a humidity calibration on the next tick, using the SHT4x reading as reference.
	/// Safe to call from any thread; the work happens on the I/O worker.
	void RequestHumidityCalibration();

	/// Start (or stop, if active) the HDC302x heater-driven decontamination
	/// cycle.  Safe to call from any thread; the work happens on the
	/// I/O worker.
	void RequestToggleDecontamination();
#endif

private:
	MeasurementWorker() = default;

	static constexpr uint32_t kMeasurementsInitialMs  = 0;
	static constexpr uint32_t kMeasurementsIntervalMs = 120'000;

	static void TickHandler(k_work *work);
	void Tick();

	static void TogglePrimaryHandler(k_work *work);
	void TogglePrimary();

#ifdef CONFIG_APP_HDC302X_MAINTENANCE_FEATURES
	static void CalibrationRequestHandler(k_work *work);
	static void ToggleDecontaminationHandler(k_work *work);
	void ToggleDecontamination();
#endif

#ifdef CONFIG_DISPLAY
	/// Re-read the HDC302x's persisted humidity offset into the cache and
	/// refresh the displayed value. The offset only changes after a
	/// successful HDC302xHumidityCalibrator::Apply(), so this is called
	/// from Init() and post-Apply rather than on every measurement tick.
	void UpdateDisplayedHumidityCalibrationOffset();

	/// Push the cached HDC302x humidity offset to the display, choosing
	/// visibility based on which sensor is currently primary. Used on
	/// sensor toggle so the indicator follows the active sensor without
	/// re-reading the I2C attribute.
	void PushHumidityCalibrationOffsetToDisplay();
#endif

	PublishFn mPublish;

	Sensor    mHdc302xSensor;
	Sensor    mSht4xSensor;
	Sensor   *mPrimarySensor   = nullptr;
	Sensor   *mSecondarySensor = nullptr;

#ifdef CONFIG_DISPLAY
	/// Cached HDC302x persisted humidity offset, in hundredths of a
	/// percent. Populated by UpdateDisplayedHumidityCalibrationOffset().
	std::optional<int16_t> mHdc302xHumidityOffsetHundredths;
#endif

#ifdef CONFIG_APP_HDC302X_MAINTENANCE_FEATURES
	HDC302xDecontaminationController mDecontaminationController;
	HDC302xHumidityCalibrator        mHumidityCalibrator;
	bool                             mCalibrationRequested = false;
#endif

	k_timer mTickTimer{};
	k_work  mTickWork{};
	k_work  mTogglePrimaryWork{};
#ifdef CONFIG_APP_HDC302X_MAINTENANCE_FEATURES
	k_work  mCalibrationRequestWork{};
	k_work  mToggleDecontaminationWork{};
#endif
};
