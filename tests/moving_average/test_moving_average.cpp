/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include <catch2/catch_test_macros.hpp>

#include "moving_average.h"

// Initialise a MovingAverage<T> to `init`, then run `steps` updates with `sample`.
template <typename T>
static T runMovingAverage(T init, T sample, int steps, int32_t alpha32 = 24)
{
	MovingAverage<T> movingAverage{ alpha32 };
	movingAverage.update(init);
	T result{};
	for (int i = 0; i < steps; ++i) {
		result = movingAverage.update(sample);
	}
	return result;
}

// ─── first-call initialisation ────────────────────────────────────────────────

TEST_CASE("MovingAverage initialises to first sample", "[moving_average][init]")
{
	REQUIRE(MovingAverage<int16_t>{ 24 }.update(2350) == 2350);
	REQUIRE(MovingAverage<int16_t>{ 24 }.update(0) == 0);
	REQUIRE(MovingAverage<int16_t>{ 24 }.update(-500) == -500);
	REQUIRE(MovingAverage<int16_t>{ 24 }.update(8000) == 8000);
}

// ─── temperature (hundredths of °C, alpha = 0.75 = 24/32) ───────────────────

TEST_CASE("MovingAverage stable positive temperature", "[moving_average][temperature]")
{
	// A constant input must leave the smoothed value unchanged.
	MovingAverage<int16_t> movingAverage{ 24 };
	movingAverage.update(2350);
	REQUIRE(movingAverage.update(2350) == 2350); // 23.50 °C
}

TEST_CASE("MovingAverage stable negative temperature", "[moving_average][temperature]")
{
	MovingAverage<int16_t> movingAverage{ 24 };
	movingAverage.update(-500);
	REQUIRE(movingAverage.update(-500) == -500); // -5.00 °C
}

TEST_CASE("MovingAverage step warming 20.00 -> 25.00 degC converges", "[moving_average][temperature]")
{
	// First step after initialisation: 0.75*2500 + 0.25*2000 = 2375
	MovingAverage<int16_t> movingAverage{ 24 };
	movingAverage.update(2000);
	REQUIRE(movingAverage.update(2500) == 2375);
	// Converges to 2500 within 5 samples (2.5 min at 30 s/sample)
	REQUIRE(runMovingAverage<int16_t>(2000, 2500, 5) == 2500);
}

TEST_CASE("MovingAverage step cooling 25.00 -> 20.00 degC converges", "[moving_average][temperature]")
{
	// First step after initialisation: 0.75*2000 + 0.25*2500 = 2125
	MovingAverage<int16_t> movingAverage{ 24 };
	movingAverage.update(2500);
	REQUIRE(movingAverage.update(2000) == 2125);
	REQUIRE(runMovingAverage<int16_t>(2500, 2000, 8) == 2000);
}

TEST_CASE("MovingAverage step crossing zero -10.00 -> 0.00 degC converges", "[moving_average][temperature]")
{
	// First step: 0.75*0 + 0.25*(-1000) = -250
	MovingAverage<int16_t> movingAverage{ 24 };
	movingAverage.update(-1000);
	REQUIRE(movingAverage.update(0) == -250);
	REQUIRE(runMovingAverage<int16_t>(-1000, 0, 8) == 0);
}

TEST_CASE("MovingAverage attenuates +-0.1 degC alternating noise", "[moving_average][temperature]")
{
	// Steady-state amplitude = 0.6 * input amplitude (alpha=0.75 theory).
	// Input: 2350 +- 10 hundredths, expected steady-state: 2350 +- 6 hundredths.
	MovingAverage<int16_t> movingAverage{ 24 };
	movingAverage.update(2350);
	int16_t last = 0;
	for (int i = 0; i < 20; ++i) {
		last = movingAverage.update((i % 2 == 0) ? 2360 : 2340);
		REQUIRE(last >= 2340);
		REQUIRE(last <= 2360);
	}
	REQUIRE(last >= 2344);
	REQUIRE(last <= 2356);
}

// ─── humidity (hundredths of %, alpha = 0.75 = 24/32) ────────────────────────

TEST_CASE("MovingAverage stable humidity", "[moving_average][humidity]")
{
	auto check = [](uint16_t val) {
		MovingAverage<uint16_t> movingAverage{ 24 };
		movingAverage.update(val);
		REQUIRE(movingAverage.update(val) == val);
	};
	check(5000); // 50.00 %
	check(50);   //  0.50 %
	check(9900); // 99.00 %
}

TEST_CASE("MovingAverage step humidity increase 40.00 -> 80.00 percent converges", "[moving_average][humidity]")
{
	// First step: 0.75*8000 + 0.25*4000 = 7000
	MovingAverage<uint16_t> movingAverage{ 24 };
	movingAverage.update(4000);
	REQUIRE(movingAverage.update(8000) == 7000);
	REQUIRE(runMovingAverage<uint16_t>(4000, 8000, 8) == 8000);
}

TEST_CASE("MovingAverage step humidity decrease 80.00 -> 40.00 percent converges", "[moving_average][humidity]")
{
	// First step: 0.75*4000 + 0.25*8000 = 5000
	MovingAverage<uint16_t> movingAverage{ 24 };
	movingAverage.update(8000);
	REQUIRE(movingAverage.update(4000) == 5000);
	REQUIRE(runMovingAverage<uint16_t>(8000, 4000, 8) == 4000);
}

TEST_CASE("MovingAverage attenuates +-0.5 percent alternating humidity noise", "[moving_average][humidity]")
{
	// Input: 5000 +- 50 hundredths, expected steady-state: 5000 +- 30 hundredths.
	MovingAverage<uint16_t> movingAverage{ 24 };
	movingAverage.update(5000);
	uint16_t last = 0;
	for (int i = 0; i < 20; ++i) {
		last = movingAverage.update((i % 2 == 0) ? 5050 : 4950);
		REQUIRE(last >= 4950);
		REQUIRE(last <= 5050);
	}
	REQUIRE(last >= 4970);
	REQUIRE(last <= 5030);
}

// ─── different alpha values ───────────────────────────────────────────────────

TEST_CASE("MovingAverage alpha 0.5 gives exact midpoint", "[moving_average][alpha]")
{
	REQUIRE(runMovingAverage<int16_t>(2000, 2400, 1, 16) == 2200); // 0.5*2400 + 0.5*2000
	REQUIRE(runMovingAverage<int16_t>(1000, 3000, 1, 16) == 2000);
}

TEST_CASE("MovingAverage alpha 0.25 weights history more heavily", "[moving_average][alpha]")
{
	// 0.25*2500 + 0.75*2000 = 2125
	REQUIRE(runMovingAverage<int16_t>(2000, 2500, 1, 8) == 2125);
}
