/*
 * SPDX-License-Identifier: LicenseRef-Apache-2.0
 */

#ifdef CONFIG_DISPLAY

#include "display_manager.h"

#include <stdio.h>
#include <system/SystemError.h>
#include <zephyr/display/cfb.h>
#include <zephyr/drivers/display.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

CHIP_ERROR DisplayManager::Init()
{
	mDev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	if (!device_is_ready(mDev)) {
		LOG_ERR("Display device not ready");
		return chip::System::MapErrorZephyr(-ENODEV);
	}
	LOG_INF("Display device is ready");

	if (display_set_pixel_format(mDev, PIXEL_FORMAT_MONO10) != 0) {
		if (display_set_pixel_format(mDev, PIXEL_FORMAT_MONO01) != 0) {
			LOG_ERR("Failed to set display pixel format");
			return CHIP_ERROR_INTERNAL;
		}
	} else {
		LOG_INF("Display supports MONO10 pixel format");
	}

	if (cfb_framebuffer_init(mDev) != 0) {
		LOG_ERR("CFB framebuffer init failed");
		return CHIP_ERROR_INTERNAL;
	}

	cfb_framebuffer_invert(mDev);
	cfb_framebuffer_clear(mDev, true);
	cfb_framebuffer_set_font(mDev, 2);
	display_blanking_off(mDev);
	mInitialized = true;
	return CHIP_NO_ERROR;
}

void DisplayManager::UpdateMeasurements(int16_t temperatureHundredths, uint16_t humidityHundredths)
{
	mCurrentTemperature = temperatureHundredths;
	mCurrentHumidity = humidityHundredths;
}

void DisplayManager::UpdateSignalStrength(bool connected, uint8_t lqi)
{
	mCurrentConnected = connected;
	mCurrentLqi = lqi;
}

void DisplayManager::RefreshDisplay()
{
	if (!mInitialized) {
		return;
	}
	if (mCurrentTemperature == mLastTemperature &&
	    mCurrentHumidity    == mLastHumidity    &&
	    mCurrentConnected   == mLastConnected   &&
	    mCurrentLqi         == mLastLqi) {
		return;
	}

	mPartialUpdateCount++;
	bool fullUpdate = (mPartialUpdateCount >= kFullUpdateInterval);
	if (fullUpdate) {
		mPartialUpdateCount = 0;
		display_blanking_on(mDev);
	}

	cfb_framebuffer_clear(mDev, false);
	DrawMeasurements();
	DrawSignalBars();
	cfb_framebuffer_finalize(mDev);

	if (fullUpdate) {
		display_blanking_off(mDev);
	}

	mLastTemperature = mCurrentTemperature;
	mLastHumidity    = mCurrentHumidity;
	mLastConnected   = mCurrentConnected;
	mLastLqi         = mCurrentLqi;
}

void DisplayManager::DrawMeasurements()
{
	constexpr uint16_t textStartY = 40;

	bool neg = (mCurrentTemperature < 0);
	int16_t absValue = neg ? -mCurrentTemperature : mCurrentTemperature;
	int16_t temperatureTenths = (absValue + 5) / 10;
	uint16_t humidityTenths = (mCurrentHumidity + 10) / 20 * 2; // 0.2% steps

	char buf[32];
	snprintf(buf, sizeof(buf), "T:%s%d.%01d C", neg ? "-" : " ", temperatureTenths / 10, temperatureTenths % 10);
	cfb_print(mDev, buf, 0, textStartY);

	snprintf(buf, sizeof(buf), "H: %d.%01d%%", humidityTenths / 10, humidityTenths % 10);
	cfb_print(mDev, buf, 0, textStartY + 50);

	snprintf(buf, sizeof(buf), "P: %d/%d", mPartialUpdateCount, kFullUpdateInterval);
	cfb_print(mDev, buf, 0, textStartY + 100);
}

void DisplayManager::DrawSignalBars()
{
	constexpr uint16_t signalBarX = 170;
	constexpr uint16_t signalBarY = 20;
	constexpr uint16_t signalBarMinimumHeight = 6;

	if (!mCurrentConnected) {
		cfb_draw_text(mDev, "-", signalBarX + 10, 7);
		return;
	}
	for (int i = 0; i < 4; i++) {
		uint16_t x = static_cast<uint16_t>(signalBarX + i * 8);
		uint16_t height = static_cast<uint16_t>(i * 4 + signalBarMinimumHeight);
		uint16_t y = static_cast<uint16_t>(signalBarY - height);
		if (i <= mCurrentLqi) {
			if (cfb_invert_area(mDev, x, y, 5, height) != 0) {
				LOG_ERR("Failed to invert area for signal bar %d", i);
			}
		} else {
			struct cfb_position start = {x, y};
			struct cfb_position end = {static_cast<uint16_t>(x + 4),
						   static_cast<uint16_t>(y + height - 1)};
			if (cfb_draw_rect(mDev, &start, &end) != 0) {
				LOG_ERR("Failed to draw rectangle for signal bar %d", i);
			}
		}
	}
}

#endif /* CONFIG_DISPLAY */
