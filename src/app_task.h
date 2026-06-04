/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "board/board.h"
#include "matter_reporter.h"

#include <platform/CHIPDeviceLayer.h>


struct Identify;

class AppTask {
public:
	static AppTask &Instance()
	{
		static AppTask sAppTask;
		return sAppTask;
	};

	CHIP_ERROR StartApp();

private:
	CHIP_ERROR Init();

	MatterReporter mMatterReporter;

	static void ButtonEventHandler(Nrf::ButtonState state, Nrf::ButtonMask hasChanged);

	static void LEDStateHandler();
};
