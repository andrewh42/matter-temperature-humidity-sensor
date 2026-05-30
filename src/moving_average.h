/*
 * SPDX-License-Identifier: LicenseRef-Apache-2.0
 */

#pragma once
#include <climits>
#include <cstdint>
#include <type_traits>

/// @brief Exponential Moving Average (EMA) filter for smoothing sensor measurements.
/// T must be int16_t or uint16_t. Restricting to 16-bit types ensures the internal
/// int32_t arithmetic (alpha32 * sample, max 32 × 65535 = 2,097,120) cannot overflow.
/// The alpha parameter controls the degree of smoothing (0 < alpha <= 1), with higher
/// values giving more weight to recent samples. alpha32 = alpha * 32.
template <typename T>
class MovingAverage {
	static_assert(std::is_same_v<T, int16_t> || std::is_same_v<T, uint16_t>,
		      "MovingAverage<T>: T must be int16_t or uint16_t");

public:
	explicit MovingAverage(int32_t alpha32) : mAlpha32(alpha32) {}

	/// Returns the updated smoothed value.
	/// On the first call the state is initialised to newSample (no blending from zero).
	T update(T newSample)
	{
		if (mSmoothed == INT32_MIN) {
			mSmoothed = newSample;
		} else {
			int32_t num = mAlpha32 * newSample + (32 - mAlpha32) * mSmoothed;
			mSmoothed = (num < 0) ? (num - 16) / 32 : (num + 16) / 32;
		}
		return static_cast<T>(mSmoothed);
	}

	/// Discards the smoothed state so the next update() re-initialises from that sample.
	void reset() { mSmoothed = INT32_MIN; }

private:
	const int32_t mAlpha32;
	int32_t mSmoothed = INT32_MIN;
};
