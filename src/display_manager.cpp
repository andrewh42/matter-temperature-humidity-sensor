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

	cfb_framebuffer_clear(mDev, true);
	cfb_framebuffer_invert(mDev);
	cfb_framebuffer_set_font(mDev, 2);
	display_blanking_off(mDev);
	mInitialized = true;
	return CHIP_NO_ERROR;
}

void DisplayManager::UpdateMeasurements(int16_t temperatureHundredths, uint16_t humidityHundredths)
{
	if (!mInitialized) {
		return;
	}
	bool neg = (temperatureHundredths < 0);
	int16_t tAbs = neg ? -temperatureHundredths : temperatureHundredths;
	int16_t tempTenths = (tAbs + 5) / 10;
	uint16_t humTenths = (humidityHundredths + 25) / 50 * 5; // Round to nearest 0.5% (matching the resolution of the sensor)
	if (tempTenths == mLastTemperature && humTenths == mLastHumidity) {
		return;
	}
	mLastTemperature = tempTenths;
	mLastHumidity = humTenths;

	mPartialUpdateCount++;
	bool fullUpdate = (mPartialUpdateCount >= kFullUpdateInterval);
	if (fullUpdate) {
		mPartialUpdateCount = 0;
	}

	char buf[32];

	if (fullUpdate) {
		display_blanking_on(mDev);
	}

	cfb_framebuffer_clear(mDev, false);

	snprintf(buf, sizeof(buf), "T:%s%d.%01d C", neg ? "-" : " ", tempTenths / 10, tempTenths % 10);
	cfb_print(mDev, buf, 0, 20);

	snprintf(buf, sizeof(buf), "H: %d.%01d%%", humTenths / 10, humTenths % 10);
	cfb_print(mDev, buf, 0, 70);

	snprintf(buf, sizeof(buf), "P: %d/%d", mPartialUpdateCount, kFullUpdateInterval);
	cfb_print(mDev, buf, 0, 120);

	cfb_framebuffer_finalize(mDev);

	if (fullUpdate) {
		display_blanking_off(mDev);
	}
}

#endif /* CONFIG_DISPLAY */
