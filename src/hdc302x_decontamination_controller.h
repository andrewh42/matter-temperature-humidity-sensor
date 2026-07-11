/*
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#ifdef CONFIG_APP_HDC302X_MAINTENANCE_FEATURES

#include <zephyr/kernel.h>
#include <cstdint>
#include <optional>

struct device;

/// HDC302x decontamination cycle: drives the on-chip heater for up to five
/// minutes (or until humidity drops below 1%) to drive out absorbed moisture.
/// While active, the host's normal measurement cadence is paused via the
/// supplied callbacks so the heater readings do not pollute the Matter cluster.
class HDC302xDecontaminationController {
public:
	HDC302xDecontaminationController() = default;
	HDC302xDecontaminationController(const HDC302xDecontaminationController &) = delete;
	HDC302xDecontaminationController &operator=(const HDC302xDecontaminationController &) = delete;

	void Init(const device *hdc302xDevice);

	void Start();
	void Stop();
	void Toggle();

	bool Active() const { return mActive; }

private:
	enum class StopReason { Toggle, TimerExpired, HumidityThresholdReached };

	static constexpr uint32_t kIntervalMs             = 2'000;
	static constexpr uint32_t kLedOnMs                = 50;
	static constexpr uint32_t kMaxDurationMs          = 5 * 60 * 1000;
	static constexpr uint16_t kHumidityExitHundredths = 100; // 1.00%
	static constexpr int32_t  kHeaterLevel            = 14;  // 100% of maximum
	static constexpr int32_t  kHeaterOff              = 0;
	static constexpr uint32_t kDisplayUpdateInterval  = 10;

	void Stop(StopReason reason);
	void RunCycle();
	void SetHeater(int32_t level);

	static const char *ToString(StopReason reason);

	const device *mDevice = nullptr;

	bool                    mActive        = false;
	int64_t                 mStartUptimeMs = 0;
	uint32_t                mCycleCount    = 0;
	std::optional<uint16_t> mLastHumidityHundredths;
	k_timer                 mTimer{};
};

#endif /* CONFIG_APP_HDC302X_MAINTENANCE_FEATURES */
