/*
 * SPDX-License-Identifier: LicenseRef-Apache-2.0
 */

#ifdef CONFIG_DISPLAY

#include "display_manager.h"

#include <system/SystemError.h>
#include <zephyr/drivers/display.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(display_manager, CONFIG_DISPLAY_MANAGER_LOG_LEVEL);

LV_FONT_DECLARE(lv_font_splinesans_medium_20);
LV_FONT_DECLARE(lv_font_splinesans_bold_72);
LV_FONT_DECLARE(lv_font_phosphor_32);

// Replace with codepoints from the Phosphor lookup script.
static constexpr uint32_t kPhosphorThermometer = 0xE5C6;
static constexpr uint32_t kPhosphorDrop        = 0xE210;

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

lv_obj_t *DisplayManager::CreateSensorCard(lv_obj_t *parent,
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
	lv_obj_set_style_pad_row(card, 11, 0);

	lv_obj_t *valueRow = lv_obj_create(card);
	lv_obj_remove_style_all(valueRow);
	lv_obj_set_size(valueRow, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
	lv_obj_set_layout(valueRow, LV_LAYOUT_FLEX);
	lv_obj_set_flex_flow(valueRow, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(valueRow, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER);
	lv_obj_set_style_pad_all(valueRow, 0, 0);
	lv_obj_set_style_pad_column(valueRow, 4, 0);

	lv_obj_t *valueLabel = lv_label_create(valueRow);
	lv_obj_set_style_text_font(valueLabel, &lv_font_splinesans_bold_72, 0);
	lv_label_set_text(valueLabel, "--.-");

	lv_obj_t *unitLabel = lv_label_create(valueRow);
	lv_obj_set_style_text_font(unitLabel, &lv_font_splinesans_medium_20, 0);
	lv_label_set_text(unitLabel, unit);

	// Bottom-align aligns bounding-box bottoms, not baselines. Lift the unit
	// label by the difference in descender space so baselines coincide.
	int32_t baselineOffset = lv_font_splinesans_bold_72.base_line -
	                         lv_font_splinesans_medium_20.base_line;
	lv_obj_set_style_pad_bottom(unitLabel, baselineOffset, 0);

	return valueLabel;
}

void DisplayManager::CreateIcon(lv_obj_t *screen, uint32_t codepoint, lv_obj_t *valueLabel)
{
	lv_obj_t *icon = lv_label_create(screen);
	lv_obj_add_flag(icon, LV_OBJ_FLAG_FLOATING);
	lv_obj_set_style_text_font(icon, &lv_font_phosphor_32, 0);
	char buf[4] = {
	    static_cast<char>(0xE0 | (codepoint >> 12)),
	    static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)),
	    static_cast<char>(0x80 | (codepoint & 0x3F)),
	    '\0'
	};
	lv_label_set_text(icon, buf);

	lv_area_t area;
	lv_obj_get_coords(valueLabel, &area);
	int32_t centreY = area.y1 + (area.y2 - area.y1) / 2;
	lv_obj_set_pos(icon, 0, centreY - lv_font_get_line_height(&lv_font_phosphor_32) / 2);
}

CHIP_ERROR DisplayManager::Init()
{
	mDev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	if (!device_is_ready(mDev)) {
		LOG_ERR("Display device not ready");
		return chip::System::MapErrorZephyr(-ENODEV);
	}
	LOG_INF("Display device is ready");

	lv_display_t *disp = lv_display_get_default();
#if CONFIG_DISPLAY_MANAGER_LOG_LEVEL >= LOG_LEVEL_DBG
	lv_draw_buf_t *drawBuf = lv_display_get_buf_active(disp);
	LOG_DBG("LVGL draw buf:\r\nsavebin framebuf.bin %p %zu\r\nexit", drawBuf->data, drawBuf->data_size);
#endif
	lv_display_set_theme(disp, lv_theme_mono_init(disp, true, &lv_font_splinesans_medium_20));

	lv_obj_t *screen = lv_screen_active();

	// Header bar
	lv_obj_t *header = lv_obj_create(screen);
	lv_obj_remove_style_all(header);
	lv_obj_set_size(header, 200, 24);
	lv_obj_set_pos(header, 0, 0);
	lv_obj_set_style_pad_right(header, 2, 0);
	lv_obj_set_style_pad_top(header, 2, 0);

	mSignalWidget = lv_obj_create(header);
	lv_obj_remove_style_all(mSignalWidget);
	lv_obj_set_size(mSignalWidget, 26, 14);
	lv_obj_align(mSignalWidget, LV_ALIGN_TOP_RIGHT, 0, 0);
	lv_obj_add_event_cb(mSignalWidget, SignalDrawCallback, LV_EVENT_DRAW_MAIN, this);

	mDecontaminationLabel = lv_label_create(header);
	lv_obj_set_style_text_font(mDecontaminationLabel, &lv_font_splinesans_medium_20, 0);
	lv_obj_align(mDecontaminationLabel, LV_ALIGN_TOP_LEFT, 4, 0);
	lv_label_set_text(mDecontaminationLabel, "");
	lv_obj_add_flag(mDecontaminationLabel, LV_OBJ_FLAG_HIDDEN);

	mSecondaryDeltaLabel = lv_label_create(header);
	lv_obj_set_style_text_font(mSecondaryDeltaLabel, &lv_font_splinesans_medium_20, 0);
	lv_obj_align(mSecondaryDeltaLabel, LV_ALIGN_TOP_LEFT, 4, 0);
	lv_label_set_text(mSecondaryDeltaLabel, "");

	// Sensor container (flex column, space-evenly)
	lv_obj_t *container = lv_obj_create(screen);
	lv_obj_remove_style_all(container);
	lv_obj_set_size(container, 200, 176);
	lv_obj_set_pos(container, 0, 24);
	lv_obj_set_layout(container, LV_LAYOUT_FLEX);
	lv_obj_set_flex_flow(container, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(container, LV_FLEX_ALIGN_SPACE_EVENLY,
	                      LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_set_style_pad_left(container, 32, 0);

	mValueTemperature = CreateSensorCard(container, "TEMPERATURE",
	                                     "\xC2\xB0" "C");

	lv_obj_t *divider = lv_obj_create(container);
	lv_obj_remove_style_all(divider);
	lv_obj_set_size(divider, 160, 1);
	lv_obj_set_style_bg_color(divider, lv_color_white(), 0);
	lv_obj_set_style_bg_opa(divider, LV_OPA_COVER, 0);

	mValueHumidity = CreateSensorCard(container, "HUMIDITY", "%");

	// Layout pass: computes card positions and value label coordinates.
	lv_obj_update_layout(screen);

	// Icons float on screen (outside the container) so they're unaffected by pad_left.
	CreateIcon(screen, kPhosphorThermometer, mValueTemperature);
	CreateIcon(screen, kPhosphorDrop,        mValueHumidity);

	mInitialized = true;
	return CHIP_NO_ERROR;
}

void DisplayManager::UpdateMeasurements(std::optional<int16_t>  temperatureHundredths,
                                        std::optional<uint16_t> humidityHundredths)
{
	mCurrentTemperature = temperatureHundredths;
	mCurrentHumidity    = humidityHundredths;
}

void DisplayManager::UpdateSignalStrength(bool connected, uint8_t lqi)
{
	mCurrentConnected = connected;
	mCurrentLqi       = lqi;
}

void DisplayManager::SetDecontaminationStatus(bool active, uint32_t elapsedSeconds)
{
	mCurrentDecontaminationActive         = active;
	mCurrentDecontaminationElapsedSeconds = elapsedSeconds;
}

void DisplayManager::SetSensorInfo(const char *secondaryName,
                                    std::optional<uint16_t> secondaryHumidityHundredths)
{
	mCurrentSecondarySensorName = secondaryName;
	mCurrentSecondaryHumidity   = secondaryHumidityHundredths;
}

void DisplayManager::RefreshDisplay()
{
	if (!mInitialized) {
		return;
	}
	if (mCurrentTemperature                   == mLastTemperature                   &&
	    mCurrentHumidity                      == mLastHumidity                      &&
	    mCurrentConnected                     == mLastConnected                     &&
	    mCurrentLqi                           == mLastLqi                           &&
	    mCurrentDecontaminationActive         == mLastDecontaminationActive         &&
	    mCurrentDecontaminationElapsedSeconds == mLastDecontaminationElapsedSeconds &&
	    mCurrentSecondarySensorName            == mLastSecondarySensorName            &&
	    mCurrentSecondaryHumidity              == mLastSecondaryHumidity) {
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
	DrawSensorInfo();
	DrawDecontamination();
	lv_timer_handler();

	if (fullUpdate) {
		display_blanking_off(mDev);
	}

	mLastTemperature                   = mCurrentTemperature;
	mLastHumidity                      = mCurrentHumidity;
	mLastConnected                     = mCurrentConnected;
	mLastLqi                           = mCurrentLqi;
	mLastDecontaminationActive         = mCurrentDecontaminationActive;
	mLastDecontaminationElapsedSeconds = mCurrentDecontaminationElapsedSeconds;
	mLastSecondarySensorName            = mCurrentSecondarySensorName;
	mLastSecondaryHumidity              = mCurrentSecondaryHumidity;
}

void DisplayManager::DrawMeasurements()
{
	if (mCurrentTemperature.has_value()) {
		int16_t value             = *mCurrentTemperature;
		bool    neg               = (value < 0);
		int16_t absValue          = neg ? -value : value;
		int16_t temperatureTenths = (absValue + 5) / 10;

		lv_label_set_text_fmt(mValueTemperature, "%s%d.%01d",
		                      neg ? "-" : "",
		                      temperatureTenths / 10, temperatureTenths % 10);
	} else {
		lv_label_set_text(mValueTemperature, "--.-");
	}

	if (mCurrentHumidity.has_value()) {
		uint16_t humidityTenths = (*mCurrentHumidity + 10) / 20 * 2; // 0.2% steps
		lv_label_set_text_fmt(mValueHumidity, "%d.%01d",
		                      humidityTenths / 10, humidityTenths % 10);
	} else {
		lv_label_set_text(mValueHumidity, "--.-");
	}
}

void DisplayManager::DrawSignalBars()
{
	lv_obj_invalidate(mSignalWidget);
}

void DisplayManager::DrawSensorInfo()
{
	if (mCurrentDecontaminationActive) {
		lv_obj_add_flag(mSecondaryDeltaLabel, LV_OBJ_FLAG_HIDDEN);
		return;
	}

	const bool secondaryValid =
		mCurrentSecondarySensorName != nullptr &&
		mCurrentSecondaryHumidity.has_value() &&
		mCurrentHumidity.has_value();
	if (secondaryValid) {
		int32_t diff = static_cast<int32_t>(*mCurrentSecondaryHumidity) -
		               static_cast<int32_t>(*mCurrentHumidity);
		int32_t deltaTenths = (diff >= 0) ? (diff + 5) / 10 : (diff - 5) / 10;
		int32_t absTenths = (deltaTenths < 0) ? -deltaTenths : deltaTenths;
		lv_obj_remove_flag(mSecondaryDeltaLabel, LV_OBJ_FLAG_HIDDEN);
		lv_label_set_text_fmt(mSecondaryDeltaLabel, "%s %s%d.%01d",
		                      mCurrentSecondarySensorName,
		                      (deltaTenths < 0) ? "-" : "+",
		                      static_cast<int>(absTenths / 10),
		                      static_cast<int>(absTenths % 10));
	} else {
		lv_obj_add_flag(mSecondaryDeltaLabel, LV_OBJ_FLAG_HIDDEN);
	}
}

void DisplayManager::DrawDecontamination()
{
	if (!mCurrentDecontaminationActive) {
		lv_obj_add_flag(mDecontaminationLabel, LV_OBJ_FLAG_HIDDEN);
		return;
	}
	lv_obj_remove_flag(mDecontaminationLabel, LV_OBJ_FLAG_HIDDEN);
	uint32_t mins = mCurrentDecontaminationElapsedSeconds / 60;
	uint32_t secs = mCurrentDecontaminationElapsedSeconds % 60;
	lv_label_set_text_fmt(mDecontaminationLabel, "DECON %u:%02u", mins, secs);
}

#endif /* CONFIG_DISPLAY */
