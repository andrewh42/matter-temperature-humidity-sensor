/*
 * SPDX-License-Identifier: LicenseRef-Apache-2.0
 */

#ifdef CONFIG_DISPLAY

#include "display_manager.h"

#include <system/SystemError.h>
#include <zephyr/drivers/display.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

LV_FONT_DECLARE(lv_font_inter_extrabold_52);
LV_FONT_DECLARE(lv_font_inter_light_12);
LV_FONT_DECLARE(lv_font_inter_regular_20);
LV_FONT_DECLARE(lv_font_phosphor_18);

// Replace with codepoints from the Phosphor lookup script.
static constexpr uint32_t kPhosphorThermometer = 0xE5C6;
static constexpr uint32_t kPhosphorDrop        = 0xE210;

// ---------------------------------------------------------------------------
// Signal widget — custom draw callback
// ---------------------------------------------------------------------------

void DisplayManager::SignalDrawCallback(lv_event_t *event)
{
	auto      *self  = static_cast<DisplayManager *>(lv_event_get_user_data(event));
	lv_layer_t *layer = lv_event_get_layer(event);
	lv_obj_t   *obj   = static_cast<lv_obj_t *>(lv_event_get_target(event));

	lv_area_t coords;
	lv_obj_get_coords(obj, &coords);

	static constexpr int32_t kBarHeights[4] = {4, 7, 10, 13};
	static constexpr int32_t kBarWidth      = 5;
	static constexpr int32_t kBarGap        = 2;

	if (!self->mCurrentConnected) {
		lv_draw_rect_dsc_t dsc;
		lv_draw_rect_dsc_init(&dsc);
		dsc.bg_color = lv_color_white();
		dsc.bg_opa   = LV_OPA_COVER;
		dsc.radius   = 0;
		int32_t totalWidth = 4 * kBarWidth + 3 * kBarGap;
		int32_t x0 = coords.x1 + (totalWidth - 10) / 2;
		int32_t y0 = (coords.y1 + coords.y2) / 2;
		lv_area_t dash = {x0, y0, x0 + 9, y0};
		lv_draw_rect(layer, &dsc, &dash);
		return;
	}

	for (int i = 0; i < 4; i++) {
		lv_draw_rect_dsc_t dsc;
		lv_draw_rect_dsc_init(&dsc);
		dsc.radius = 0;

		int32_t x = coords.x1 + i * (kBarWidth + kBarGap);
		int32_t h = kBarHeights[i];
		lv_area_t bar = {x, coords.y2 - h, x + kBarWidth - 1, coords.y2};

		if (i <= self->mCurrentLqi) {
			dsc.bg_color     = lv_color_white();
			dsc.bg_opa       = LV_OPA_COVER;
			dsc.border_width = 0;
		} else {
			dsc.bg_opa       = LV_OPA_TRANSP;
			dsc.border_width = 1;
			dsc.border_color = lv_color_white();
			dsc.border_opa   = LV_OPA_COVER;
		}
		lv_draw_rect(layer, &dsc, &bar);
	}
}

// ---------------------------------------------------------------------------
// CreateSensorCard — builds one sensor card, returns the value label
// ---------------------------------------------------------------------------

lv_obj_t *DisplayManager::CreateSensorCard(lv_obj_t *parent,
                                            uint32_t   iconCodepoint,
                                            const char *title,
                                            const char *unit)
{
	lv_obj_t *card = lv_obj_create(parent);
	lv_obj_remove_style_all(card);
	lv_obj_set_width(card, LV_PCT(100));
	lv_obj_set_height(card, LV_SIZE_CONTENT);
	lv_obj_set_layout(card, LV_LAYOUT_FLEX);
	lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(card, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_set_style_pad_all(card, 0, 0);
	lv_obj_set_style_pad_row(card, 0, 0);

	lv_obj_t *row = lv_obj_create(card);
	lv_obj_remove_style_all(row);
	lv_obj_set_size(row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
	lv_obj_set_layout(row, LV_LAYOUT_FLEX);
	lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_set_style_pad_all(row, 0, 0);
	lv_obj_set_style_pad_column(row, 6, 0);

	lv_obj_t *icon = lv_label_create(row);
	lv_obj_set_style_text_font(icon, &lv_font_phosphor_18, 0);
	char iconBuf[8];
	lv_snprintf(iconBuf, sizeof(iconBuf), "%lc", iconCodepoint);
	lv_label_set_text(icon, iconBuf);

	lv_obj_t *titleLabel = lv_label_create(row);
	lv_obj_set_style_text_font(titleLabel, &lv_font_inter_light_12, 0);
	lv_obj_set_style_text_letter_space(titleLabel, 2, 0);
	lv_label_set_text(titleLabel, title);

	lv_obj_t *valueLabel = lv_label_create(card);
	lv_obj_set_style_text_font(valueLabel, &lv_font_inter_extrabold_52, 0);
	lv_obj_set_style_pad_top(valueLabel, 2, 0);
	lv_label_set_text(valueLabel, "--.-");

	lv_obj_t *unitLabel = lv_label_create(card);
	lv_obj_set_style_text_font(unitLabel, &lv_font_inter_regular_20, 0);
	lv_obj_set_style_pad_bottom(unitLabel, 4, 0);
	lv_label_set_text(unitLabel, unit);

	return valueLabel;
}

// ---------------------------------------------------------------------------
// Init
// ---------------------------------------------------------------------------

CHIP_ERROR DisplayManager::Init()
{
	mDev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	if (!device_is_ready(mDev)) {
		LOG_ERR("Display device not ready");
		return chip::System::MapErrorZephyr(-ENODEV);
	}
	LOG_INF("Display device is ready");

	lv_display_t *disp = lv_display_get_default();
	lv_display_set_theme(disp, lv_theme_mono_init(disp, true, &lv_font_inter_extrabold_52));

	lv_obj_t *screen = lv_screen_active();

	// Header bar
	lv_obj_t *header = lv_obj_create(screen);
	lv_obj_remove_style_all(header);
	lv_obj_set_size(header, 200, 24);
	lv_obj_set_pos(header, 0, 0);
	lv_obj_set_style_pad_right(header, 6, 0);
	lv_obj_set_style_pad_top(header, 5, 0);

	mSignalWidget = lv_obj_create(header);
	lv_obj_remove_style_all(mSignalWidget);
	lv_obj_set_size(mSignalWidget, 32, 16);
	lv_obj_align(mSignalWidget, LV_ALIGN_RIGHT_MID, 0, 0);
	lv_obj_add_event_cb(mSignalWidget, SignalDrawCallback, LV_EVENT_DRAW_MAIN, this);

	// Sensor container (flex column, space-evenly)
	lv_obj_t *container = lv_obj_create(screen);
	lv_obj_remove_style_all(container);
	lv_obj_set_size(container, 200, 176);
	lv_obj_set_pos(container, 0, 24);
	lv_obj_set_layout(container, LV_LAYOUT_FLEX);
	lv_obj_set_flex_flow(container, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(container, LV_FLEX_ALIGN_SPACE_EVENLY,
	                      LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_set_style_pad_hor(container, 16, 0);

	mValueTemperature = CreateSensorCard(container, kPhosphorThermometer,
	                                     "TEMPERATURE", "\xC2\xB0" "C");

	lv_obj_t *divider = lv_obj_create(container);
	lv_obj_remove_style_all(divider);
	lv_obj_set_size(divider, 160, 1);
	lv_obj_set_style_bg_color(divider, lv_color_white(), 0);
	lv_obj_set_style_bg_opa(divider, LV_OPA_COVER, 0);

	mValueHumidity = CreateSensorCard(container, kPhosphorDrop,
	                                  "HUMIDITY", "%RH");

	mInitialized = true;
	return CHIP_NO_ERROR;
}

// ---------------------------------------------------------------------------
// State setters
// ---------------------------------------------------------------------------

void DisplayManager::UpdateMeasurements(int16_t temperatureHundredths,
                                        uint16_t humidityHundredths)
{
	mCurrentTemperature = temperatureHundredths;
	mCurrentHumidity    = humidityHundredths;
}

void DisplayManager::UpdateSignalStrength(bool connected, uint8_t lqi)
{
	mCurrentConnected = connected;
	mCurrentLqi       = lqi;
}

// ---------------------------------------------------------------------------
// Refresh
// ---------------------------------------------------------------------------

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

	DrawMeasurements();
	DrawSignalBars();
	lv_timer_handler();

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
	bool    neg               = (mCurrentTemperature < 0);
	int16_t absValue          = neg ? -mCurrentTemperature : mCurrentTemperature;
	int16_t temperatureTenths = (absValue + 5) / 10;

	lv_label_set_text_fmt(mValueTemperature, "%s%d.%01d",
	                      neg ? "-" : "",
	                      temperatureTenths / 10, temperatureTenths % 10);

	uint16_t humidityTenths = (mCurrentHumidity + 10) / 20 * 2; // 0.2% steps
	lv_label_set_text_fmt(mValueHumidity, "%d.%01d",
	                      humidityTenths / 10, humidityTenths % 10);
}

void DisplayManager::DrawSignalBars()
{
	lv_obj_invalidate(mSignalWidget);
}

#endif /* CONFIG_DISPLAY */
