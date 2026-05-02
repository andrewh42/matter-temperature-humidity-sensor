/*
 * SPDX-License-Identifier: LicenseRef-Apache-2.0
 */

#include <catch2/catch_test_macros.hpp>

#include "ema.h"

// Initialise an Ema to `init`, then run `steps` updates with `sample`.
static int32_t runEma(int32_t init, int32_t sample, int steps, int32_t alpha32 = 24)
{
	Ema ema{ alpha32 };
	ema.update(init);
	int32_t result = 0;
	for (int i = 0; i < steps; ++i) {
		result = ema.update(sample);
	}
	return result;
}

// ─── first-call initialisation ────────────────────────────────────────────────

TEST_CASE("Ema initialises to first sample", "[ema][init]")
{
	REQUIRE(Ema{ 24 }.update(2350) == 2350);
	REQUIRE(Ema{ 24 }.update(0) == 0);
	REQUIRE(Ema{ 24 }.update(-500) == -500);
	REQUIRE(Ema{ 24 }.update(8000) == 8000);
}

// ─── temperature (hundredths of °C, alpha = 0.75 = 24/32) ───────────────────

TEST_CASE("Ema stable positive temperature", "[ema][temperature]")
{
	// A constant input must leave the smoothed value unchanged.
	Ema ema{ 24 };
	ema.update(2350);
	REQUIRE(ema.update(2350) == 2350); // 23.50 °C
}

TEST_CASE("Ema stable negative temperature", "[ema][temperature]")
{
	Ema ema{ 24 };
	ema.update(-500);
	REQUIRE(ema.update(-500) == -500); // -5.00 °C
}

TEST_CASE("Ema step warming 20.00 -> 25.00 degC converges", "[ema][temperature]")
{
	// First EMA step after initialisation: 0.75*2500 + 0.25*2000 = 2375
	Ema ema{ 24 };
	ema.update(2000);
	REQUIRE(ema.update(2500) == 2375);
	// Converges to 2500 within 5 samples (2.5 min at 30 s/sample)
	REQUIRE(runEma(2000, 2500, 5) == 2500);
}

TEST_CASE("Ema step cooling 25.00 -> 20.00 degC converges", "[ema][temperature]")
{
	// First EMA step after initialisation: 0.75*2000 + 0.25*2500 = 2125
	Ema ema{ 24 };
	ema.update(2500);
	REQUIRE(ema.update(2000) == 2125);
	REQUIRE(runEma(2500, 2000, 8) == 2000);
}

TEST_CASE("Ema step crossing zero -10.00 -> 0.00 degC converges", "[ema][temperature]")
{
	// First EMA step: 0.75*0 + 0.25*(-1000) = -250
	Ema ema{ 24 };
	ema.update(-1000);
	REQUIRE(ema.update(0) == -250);
	REQUIRE(runEma(-1000, 0, 8) == 0);
}

TEST_CASE("Ema attenuates +-0.1 degC alternating noise", "[ema][temperature]")
{
	// Steady-state amplitude = 0.6 * input amplitude (alpha=0.75 theory).
	// Input: 2350 +- 10 hundredths, expected steady-state: 2350 +- 6 hundredths.
	Ema ema{ 24 };
	ema.update(2350);
	int32_t last = 0;
	for (int i = 0; i < 20; ++i) {
		last = ema.update((i % 2 == 0) ? 2360 : 2340);
		REQUIRE(last >= 2340);
		REQUIRE(last <= 2360);
	}
	REQUIRE(last >= 2344);
	REQUIRE(last <= 2356);
}

// ─── humidity (hundredths of %, alpha = 0.75 = 24/32) ────────────────────────

TEST_CASE("Ema stable humidity", "[ema][humidity]")
{
	auto check = [](int32_t val) {
		Ema ema{ 24 };
		ema.update(val);
		REQUIRE(ema.update(val) == val);
	};
	check(5000); // 50.00 %
	check(50);   //  0.50 %
	check(9900); // 99.00 %
}

TEST_CASE("Ema step humidity increase 40.00 -> 80.00 percent converges", "[ema][humidity]")
{
	// First EMA step: 0.75*8000 + 0.25*4000 = 7000
	Ema ema{ 24 };
	ema.update(4000);
	REQUIRE(ema.update(8000) == 7000);
	REQUIRE(runEma(4000, 8000, 8) == 8000);
}

TEST_CASE("Ema step humidity decrease 80.00 -> 40.00 percent converges", "[ema][humidity]")
{
	// First EMA step: 0.75*4000 + 0.25*8000 = 5000
	Ema ema{ 24 };
	ema.update(8000);
	REQUIRE(ema.update(4000) == 5000);
	REQUIRE(runEma(8000, 4000, 8) == 4000);
}

TEST_CASE("Ema attenuates +-0.5 percent alternating humidity noise", "[ema][humidity]")
{
	// Input: 5000 +- 50 hundredths, expected steady-state: 5000 +- 30 hundredths.
	Ema ema{ 24 };
	ema.update(5000);
	int32_t last = 0;
	for (int i = 0; i < 20; ++i) {
		last = ema.update((i % 2 == 0) ? 5050 : 4950);
		REQUIRE(last >= 4950);
		REQUIRE(last <= 5050);
	}
	REQUIRE(last >= 4970);
	REQUIRE(last <= 5030);
}

// ─── different alpha values ───────────────────────────────────────────────────

TEST_CASE("Ema alpha 0.5 gives exact midpoint", "[ema][alpha]")
{
	REQUIRE(runEma(2000, 2400, 1, 16) == 2200); // 0.5*2400 + 0.5*2000
	REQUIRE(runEma(1000, 3000, 1, 16) == 2000);
}

TEST_CASE("Ema alpha 0.25 weights history more heavily", "[ema][alpha]")
{
	// 0.25*2500 + 0.75*2000 = 2125
	REQUIRE(runEma(2000, 2500, 1, 8) == 2125);
}
