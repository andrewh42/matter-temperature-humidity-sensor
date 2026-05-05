/*
 * SPDX-License-Identifier: LicenseRef-Apache-2.0
 */

#ifdef CONFIG_DISPLAY

#include "display_manager.h"

#include <system/SystemError.h>
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

	// LVGL is initialised by LV_Z_AUTO_INIT (SYS_INIT at priority 90) before
	// app code runs. Widgets are created on the default screen.
	lv_obj_t *screen = lv_screen_active();

	mLabelTemperature = lv_label_create(screen);
	lv_obj_set_pos(mLabelTemperature, 0, 40);
	lv_obj_set_style_text_font(mLabelTemperature, &lv_font_montserrat_20, 0);
	lv_label_set_text(mLabelTemperature, "T:  -.- C");

	mLabelHumidity = lv_label_create(screen);
	lv_obj_set_pos(mLabelHumidity, 0, 90);
	lv_obj_set_style_text_font(mLabelHumidity, &lv_font_montserrat_20, 0);
	lv_label_set_text(mLabelHumidity, "H:  -.- %");

	mLabelPartial = lv_label_create(screen);
	lv_obj_set_pos(mLabelPartial, 0, 140);
	lv_obj_set_style_text_font(mLabelPartial, &lv_font_montserrat_20, 0);
	lv_label_set_text(mLabelPartial, "P: 0/30");

	// Disconnected indicator: "-" in the signal-bar area (hidden until first refresh)
	mLabelDisconnected = lv_label_create(screen);
	lv_obj_set_pos(mLabelDisconnected, 180, 7);
	lv_label_set_text(mLabelDisconnected, "-");
	lv_obj_add_flag(mLabelDisconnected, LV_OBJ_FLAG_HIDDEN);

	// Signal bars: bar i at x=170+i*8, height=i*4+6, y=20-height, width=5
	for (int i = 0; i < 4; i++) {
		int32_t height = i * 4 + 6;
		mSignalBars[i] = lv_obj_create(screen);
		lv_obj_remove_style_all(mSignalBars[i]);
		lv_obj_set_pos(mSignalBars[i], 170 + i * 8, 20 - height);
		lv_obj_set_size(mSignalBars[i], 5, height);
		lv_obj_set_style_radius(mSignalBars[i], 0, 0);
		lv_obj_set_style_pad_all(mSignalBars[i], 0, 0);
		// Default: outline style (inactive bar); DrawSignalBars() fills active bars
		lv_obj_set_style_bg_opa(mSignalBars[i],       LV_OPA_TRANSP,    0);
		lv_obj_set_style_border_width(mSignalBars[i], 1,                0);
		lv_obj_set_style_border_color(mSignalBars[i], lv_color_black(), 0);
		lv_obj_set_style_border_opa(mSignalBars[i],   LV_OPA_COVER,     0);
		lv_obj_add_flag(mSignalBars[i], LV_OBJ_FLAG_HIDDEN);
	}

	mInitialized = true;
	return CHIP_NO_ERROR;
}

void DisplayManager::UpdateMeasurements(int16_t temperatureHundredths, uint16_t humidityHundredths)
{
	mCurrentTemperature = temperatureHundredths;
	mCurrentHumidity    = humidityHundredths;
}

void DisplayManager::UpdateSignalStrength(bool connected, uint8_t lqi)
{
	mCurrentConnected = connected;
	mCurrentLqi       = lqi;
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
		// Sets SSD16XX to full-waveform profile and defers display_write().
		display_blanking_on(mDev);
	}

	DrawMeasurements();
	DrawSignalBars();
	lv_timer_handler(); // renders dirty widgets → calls display_write() via flush callback

	if (fullUpdate) {
		// Calls ssd16xx_update_display() to trigger the full-waveform panel refresh.
		display_blanking_off(mDev);
	}

	mLastTemperature = mCurrentTemperature;
	mLastHumidity    = mCurrentHumidity;
	mLastConnected   = mCurrentConnected;
	mLastLqi         = mCurrentLqi;
}

void DisplayManager::DrawMeasurements()
{
	bool     neg               = (mCurrentTemperature < 0);
	int16_t  absValue          = neg ? -mCurrentTemperature : mCurrentTemperature;
	int16_t  temperatureTenths = (absValue + 5) / 10;
	uint16_t humidityTenths    = (mCurrentHumidity + 10) / 20 * 2; // 0.2% steps

	lv_label_set_text_fmt(mLabelTemperature, "T:%s%d.%01d C",
	                      neg ? "-" : " ",
	                      temperatureTenths / 10, temperatureTenths % 10);

	lv_label_set_text_fmt(mLabelHumidity, "H: %d.%01d%%",
	                      humidityTenths / 10, humidityTenths % 10);

	lv_label_set_text_fmt(mLabelPartial, "P: %d/%d",
	                      mPartialUpdateCount, kFullUpdateInterval);
}

void DisplayManager::DrawSignalBars()
{
	if (!mCurrentConnected) {
		lv_obj_remove_flag(mLabelDisconnected, LV_OBJ_FLAG_HIDDEN);
		for (int i = 0; i < 4; i++) {
			lv_obj_add_flag(mSignalBars[i], LV_OBJ_FLAG_HIDDEN);
		}
		return;
	}

	lv_obj_add_flag(mLabelDisconnected, LV_OBJ_FLAG_HIDDEN);

	for (int i = 0; i < 4; i++) {
		lv_obj_remove_flag(mSignalBars[i], LV_OBJ_FLAG_HIDDEN);
		if (i <= mCurrentLqi) {
			// Filled (active) bar
			lv_obj_set_style_bg_opa(mSignalBars[i],       LV_OPA_COVER,    0);
			lv_obj_set_style_bg_color(mSignalBars[i],     lv_color_black(), 0);
			lv_obj_set_style_border_width(mSignalBars[i], 0,               0);
		} else {
			// Outline (inactive) bar
			lv_obj_set_style_bg_opa(mSignalBars[i],       LV_OPA_TRANSP,   0);
			lv_obj_set_style_border_width(mSignalBars[i], 1,               0);
		}
	}
}

#endif /* CONFIG_DISPLAY */
