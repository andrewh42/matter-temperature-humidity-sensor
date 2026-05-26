/*
 * SPDX-License-Identifier: LicenseRef-Apache-2.0
 */

#pragma once

#include <zephyr/kernel.h>
#include <cstdint>

struct device;

/// HDC302x decontamination cycle: drives the on-chip heater for up to five
/// minutes (or until humidity drops below 1%) to drive out absorbed moisture.
/// While active, the host's normal measurement cadence is paused via the
/// supplied callbacks so the heater readings do not pollute the Matter cluster.
class DecontaminationController {
public:
	using Callback = void (*)(void *context);

	DecontaminationController() = default;
	DecontaminationController(const DecontaminationController &) = delete;
	DecontaminationController &operator=(const DecontaminationController &) = delete;

	void Init(const device *hdc302xDevice,
	          Callback onStarted,
	          Callback onStopped,
	          void *callbackContext);

	void Start();
	void Stop();
	void Toggle();

	bool Active() const { return mActive; }

private:
	static constexpr uint32_t kIntervalMs             = 2'000;
	static constexpr uint32_t kLedOnMs                = 50;
	static constexpr uint32_t kMaxDurationMs          = 5 * 60 * 1000;
	static constexpr uint16_t kHumidityExitHundredths = 100; // 1.00%
	static constexpr int32_t  kHeaterLevel            = 14;  // 100% of maximum
	static constexpr int32_t  kHeaterOff              = 0;

	void RunCycle();
	void SetHeater(int32_t level);

	const device *mDevice          = nullptr;
	Callback      mOnStarted       = nullptr;
	Callback      mOnStopped       = nullptr;
	void         *mCallbackContext = nullptr;

	bool    mActive        = false;
	int64_t mStartUptimeMs = 0;
	k_timer mTimer{};
};
