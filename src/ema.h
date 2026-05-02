/*
 * SPDX-License-Identifier: LicenseRef-Apache-2.0
 */

#pragma once
#include <climits>
#include <cstdint>

/// @brief Exponential Moving Average (EMA) filter for smoothing sensor measurements.
/// The alpha parameter controls the degree of smoothing (0 < alpha <= 1), with higher values giving more weight to recent samples.
/// The implementation uses integer arithmetic with a fixed-point representation of alpha (alpha32 = alpha * 32) to avoid floating-point calculations.
class Ema {
public:
	explicit Ema(int32_t alpha32) : mAlpha32(alpha32) {}

	// Returns the updated smoothed value.
	// On the first call the state is initialised to newSample (no blending from zero).
	int32_t update(int32_t newSample)
	{
		if (mSmoothed == INT32_MIN) {
			mSmoothed = newSample;
		} else {
			int32_t num = mAlpha32 * newSample + (32 - mAlpha32) * mSmoothed;
			mSmoothed = (num < 0) ? (num - 16) / 32 : (num + 16) / 32;
		}
		return mSmoothed;
	}

private:
	const int32_t mAlpha32;
	int32_t mSmoothed = INT32_MIN;
};
