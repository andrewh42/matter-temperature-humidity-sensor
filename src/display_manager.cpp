/*
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef CONFIG_DISPLAY

#include "display_manager.h"

#include "io_worker.h"

#include <zephyr/drivers/display.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(display_manager, CONFIG_DISPLAY_MANAGER_LOG_LEVEL);

LV_FONT_DECLARE(lv_font_splinesans_medium_18);
LV_FONT_DECLARE(lv_font_splinesans_medium_20);
LV_FONT_DECLARE(lv_font_splinesans_bold_72);
LV_FONT_DECLARE(lv_font_phosphor_bold_18);
LV_FONT_DECLARE(lv_font_phosphor_32);

// Replace with codepoints from the Phosphor lookup script.
static constexpr uint32_t kPhosphorThermometer = 0xE5C6;
static constexpr uint32_t kPhosphorDrop        = 0xE210;
static constexpr uint32_t kPhosphorArrowsUpDown = 0xEB04;

static constexpr int32_t kScreenWidth            = 200;
static constexpr int32_t kScreenHeight           = 200;
static constexpr int32_t kHeaderHeight           = 24;
static constexpr int32_t kHeaderInnerPadRight    = 2;
static constexpr int32_t kHeaderInnerPadTop      = 2;
static constexpr int32_t kHeaderTextLeftOffset   = 0;
static constexpr int32_t kSignalWidgetWidth      = 26;
static constexpr int32_t kSignalWidgetHeight     = 14;
static constexpr int32_t kStatusBarColumnGap     = 4;
static constexpr int32_t kSensorContainerHeight  = kScreenHeight - kHeaderHeight;
static constexpr int32_t kSensorContainerLeftPad = 32;
static constexpr int32_t kDividerWidth           = 160;
static constexpr int32_t kDividerHeight          = 1;

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

void DisplayManager::Init()
{
	k_work_init(&mInitWork, InitHandler);
	k_work_submit_to_queue(&IoWorker::Instance().Queue(), &mInitWork);
}

void DisplayManager::InitHandler(k_work *)
{
	Instance().InitOnWorker();
}

void DisplayManager::InitOnWorker()
{
	if (!InitDisplayDevice()) {
		mState = State::Unavailable;
		return;
	}
	BuildUserInterface();
	mState = State::Ready;
}

bool DisplayManager::InitDisplayDevice()
{
	mDev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	if (!device_is_ready(mDev)) {
		LOG_ERR("Display device not ready; continuing without display");
		return false;
	}
	LOG_INF("Display device is ready");
	return true;
}

void DisplayManager::BuildUserInterface()
{
	lv_display_t *disp = lv_display_get_default();
#if CONFIG_DISPLAY_MANAGER_LOG_LEVEL >= LOG_LEVEL_DBG
	lv_draw_buf_t *drawBuf = lv_display_get_buf_active(disp);
	LOG_DBG("LVGL draw buf:\r\nsavebin framebuf.bin %p %zu\r\nexit", drawBuf->data, drawBuf->data_size);
#endif
	lv_display_set_theme(disp, lv_theme_mono_init(disp, true, &lv_font_splinesans_medium_20));

	lv_obj_t *screen = lv_screen_active();
	BuildStatusBar(screen);
	BuildMeasurementsContainer(screen);

	// Layout pass: computes card positions and value label coordinates
	// so the floating icons can be placed relative to them.
	lv_obj_update_layout(screen);
	PlaceFloatingIcons(screen);
}

void DisplayManager::BuildStatusBar(lv_obj_t *screen)
{
	lv_obj_t *header = lv_obj_create(screen);
	lv_obj_remove_style_all(header);
	lv_obj_set_size(header, kScreenWidth, kHeaderHeight);
	lv_obj_set_pos(header, 0, 0);
	lv_obj_set_style_pad_right(header, kHeaderInnerPadRight, 0);
	lv_obj_set_style_pad_top(header, kHeaderInnerPadTop, 0);

	mSignalWidget = lv_obj_create(header);
	lv_obj_remove_style_all(mSignalWidget);
	lv_obj_set_size(mSignalWidget, kSignalWidgetWidth, kSignalWidgetHeight);
	lv_obj_align(mSignalWidget, LV_ALIGN_TOP_RIGHT, 0, 0);
	lv_obj_add_event_cb(mSignalWidget, SignalDrawCallback, LV_EVENT_DRAW_MAIN, this);

	mDecontaminationLabel = lv_label_create(header);
	lv_obj_set_style_text_font(mDecontaminationLabel, &lv_font_splinesans_medium_18, 0);
	lv_obj_align(mDecontaminationLabel, LV_ALIGN_TOP_LEFT, kHeaderTextLeftOffset, 0);
	lv_label_set_text(mDecontaminationLabel, "");
	lv_obj_add_flag(mDecontaminationLabel, LV_OBJ_FLAG_HIDDEN);

	mStatusBarContainer = lv_obj_create(header);
	lv_obj_remove_style_all(mStatusBarContainer);
	lv_obj_set_size(mStatusBarContainer, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
	lv_obj_align(mStatusBarContainer, LV_ALIGN_TOP_LEFT, kHeaderTextLeftOffset, 0);
	lv_obj_set_layout(mStatusBarContainer, LV_LAYOUT_FLEX);
	lv_obj_set_flex_flow(mStatusBarContainer, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(mStatusBarContainer, LV_FLEX_ALIGN_START,
	                      LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_set_style_pad_column(mStatusBarContainer, kStatusBarColumnGap, 0);

	mPrimarySensorLabel = lv_label_create(mStatusBarContainer);
	lv_obj_set_style_text_font(mPrimarySensorLabel, &lv_font_splinesans_medium_18, 0);
	lv_label_set_text(mPrimarySensorLabel, "");
	lv_obj_add_flag(mPrimarySensorLabel, LV_OBJ_FLAG_HIDDEN);

	mOffsetFromSecondaryLabel = lv_label_create(mStatusBarContainer);
	lv_obj_set_style_text_font(mOffsetFromSecondaryLabel, &lv_font_splinesans_medium_18, 0);
	lv_label_set_text(mOffsetFromSecondaryLabel, "");
	lv_obj_add_flag(mOffsetFromSecondaryLabel, LV_OBJ_FLAG_HIDDEN);

	mCalibrationOffsetContainer = lv_obj_create(mStatusBarContainer);
	lv_obj_remove_style_all(mCalibrationOffsetContainer);
	lv_obj_set_size(mCalibrationOffsetContainer, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
	lv_obj_set_style_margin_left(mCalibrationOffsetContainer, -4, 0);
	lv_obj_set_layout(mCalibrationOffsetContainer, LV_LAYOUT_FLEX);
	lv_obj_set_flex_flow(mCalibrationOffsetContainer, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(mCalibrationOffsetContainer, LV_FLEX_ALIGN_START,
	                      LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_set_style_pad_column(mCalibrationOffsetContainer, 0, 0);
	lv_obj_add_flag(mCalibrationOffsetContainer, LV_OBJ_FLAG_HIDDEN);

	mCalibrationOffsetGlyph = lv_label_create(mCalibrationOffsetContainer);
	lv_obj_set_style_text_font(mCalibrationOffsetGlyph, &lv_font_phosphor_bold_18, 0);
	char arrowsBuf[4] = {
	    static_cast<char>(0xE0 | (kPhosphorArrowsUpDown >> 12)),
	    static_cast<char>(0x80 | ((kPhosphorArrowsUpDown >> 6) & 0x3F)),
	    static_cast<char>(0x80 | (kPhosphorArrowsUpDown & 0x3F)),
	    '\0'
	};
	lv_label_set_text(mCalibrationOffsetGlyph, arrowsBuf);

	mCalibrationOffsetValue = lv_label_create(mCalibrationOffsetContainer);
	lv_obj_set_style_text_font(mCalibrationOffsetValue, &lv_font_splinesans_medium_18, 0);
	lv_obj_set_style_margin_left(mCalibrationOffsetValue, -6, 0);
	lv_label_set_text(mCalibrationOffsetValue, "");
}

void DisplayManager::BuildMeasurementsContainer(lv_obj_t *screen)
{
	lv_obj_t *container = lv_obj_create(screen);
	lv_obj_remove_style_all(container);
	lv_obj_set_size(container, kScreenWidth, kSensorContainerHeight);
	lv_obj_set_pos(container, 0, kHeaderHeight);
	lv_obj_set_layout(container, LV_LAYOUT_FLEX);
	lv_obj_set_flex_flow(container, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(container, LV_FLEX_ALIGN_SPACE_EVENLY,
	                      LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_set_style_pad_left(container, kSensorContainerLeftPad, 0);

	mValueTemperature = CreateSensorCard(container, "TEMPERATURE",
	                                     "\xC2\xB0" "C");

	lv_obj_t *divider = lv_obj_create(container);
	lv_obj_remove_style_all(divider);
	lv_obj_set_size(divider, kDividerWidth, kDividerHeight);
	lv_obj_set_style_bg_color(divider, lv_color_white(), 0);
	lv_obj_set_style_bg_opa(divider, LV_OPA_COVER, 0);

	mValueHumidity = CreateSensorCard(container, "HUMIDITY", "%");
}

void DisplayManager::PlaceFloatingIcons(lv_obj_t *screen)
{
	// Icons float on screen (outside the sensor container) so they're
	// unaffected by its pad_left.
	CreateIcon(screen, kPhosphorThermometer, mValueTemperature);
	CreateIcon(screen, kPhosphorDrop,        mValueHumidity);
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

void DisplayManager::SetPrimarySensorName(const char *name)
{
	mCurrentPrimarySensorName = name;
}

void DisplayManager::SetSecondarySensorPresent(bool present)
{
	mSecondarySensorPresent = present;
}

void DisplayManager::SetSecondaryHumidity(std::optional<uint16_t> humidityHundredths)
{
	mCurrentSecondaryHumidity = humidityHundredths;
}

void DisplayManager::SetHumidityCalibrationOffset(std::optional<int16_t> offsetHundredths)
{
	mCurrentHumidityCalibrationOffset = offsetHundredths;
}

void DisplayManager::RefreshDisplay()
{
	if (mState != State::Ready) {
		return;
	}

	const bool sensorInfoTemporarilyVisible =
		mSecondarySensorPresent || mTemporarySensorInfoRefreshesRemaining > 0;
	const bool primaryNameVisible =
		(mCurrentPrimarySensorName != nullptr) && sensorInfoTemporarilyVisible;
	const bool calibrationOffsetVisible =
		mCurrentHumidityCalibrationOffset.has_value() && sensorInfoTemporarilyVisible;

	const bool noChange =
		mCurrentTemperature                   == mLastTemperature                   &&
		mCurrentHumidity                      == mLastHumidity                      &&
		mCurrentConnected                     == mLastConnected                     &&
		mCurrentLqi                           == mLastLqi                           &&
		mCurrentDecontaminationActive         == mLastDecontaminationActive         &&
		mCurrentDecontaminationElapsedSeconds == mLastDecontaminationElapsedSeconds &&
		mCurrentPrimarySensorName             == mLastPrimarySensorName             &&
		mCurrentSecondaryHumidity             == mLastSecondaryHumidity             &&
		mCurrentHumidityCalibrationOffset     == mLastHumidityCalibrationOffset     &&
		primaryNameVisible                    == mLastPrimaryNameVisible            &&
		calibrationOffsetVisible              == mLastCalibrationOffsetVisible;

	if (!noChange) {
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
		mLastPrimarySensorName             = mCurrentPrimarySensorName;
		mLastSecondaryHumidity             = mCurrentSecondaryHumidity;
		mLastHumidityCalibrationOffset     = mCurrentHumidityCalibrationOffset;
		mLastPrimaryNameVisible            = primaryNameVisible;
		mLastCalibrationOffsetVisible      = calibrationOffsetVisible;
	}

	// Each refresh in single-sensor mode (decon inactive) consumes one of
	// the budgeted display updates; pausing during decon prevents the
	// overlay from silently eating the visibility window.
	if (!mSecondarySensorPresent && !mCurrentDecontaminationActive &&
	    mTemporarySensorInfoRefreshesRemaining > 0) {
		mTemporarySensorInfoRefreshesRemaining--;
	}
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
	const bool sensorInfoTemporarilyVisible =
		mSecondarySensorPresent || mTemporarySensorInfoRefreshesRemaining > 0;

	const bool primaryNameVisible =
		(mCurrentPrimarySensorName != nullptr) && sensorInfoTemporarilyVisible;
	if (primaryNameVisible) {
		lv_label_set_text(mPrimarySensorLabel, mCurrentPrimarySensorName);
		lv_obj_remove_flag(mPrimarySensorLabel, LV_OBJ_FLAG_HIDDEN);
	} else {
		lv_obj_add_flag(mPrimarySensorLabel, LV_OBJ_FLAG_HIDDEN);
	}

	if (mCurrentHumidity.has_value() && mCurrentSecondaryHumidity.has_value()) {
		int32_t diff = static_cast<int32_t>(*mCurrentHumidity) -
		               static_cast<int32_t>(*mCurrentSecondaryHumidity);
		int32_t deltaTenths = (diff >= 0) ? (diff + 5) / 10 : (diff - 5) / 10;
		int32_t absTenths = (deltaTenths < 0) ? -deltaTenths : deltaTenths;
		lv_label_set_text_fmt(mOffsetFromSecondaryLabel, "%s%d.%01d",
		                      (deltaTenths < 0) ? "-" : "+",
		                      static_cast<int>(absTenths / 10),
		                      static_cast<int>(absTenths % 10));
		lv_obj_remove_flag(mOffsetFromSecondaryLabel, LV_OBJ_FLAG_HIDDEN);
	} else {
		lv_obj_add_flag(mOffsetFromSecondaryLabel, LV_OBJ_FLAG_HIDDEN);
	}

	const bool calibrationOffsetVisible =
		mCurrentHumidityCalibrationOffset.has_value() && sensorInfoTemporarilyVisible;
	if (calibrationOffsetVisible) {
		int32_t value     = *mCurrentHumidityCalibrationOffset;
		int32_t tenths    = (value >= 0) ? (value + 5) / 10 : (value - 5) / 10;
		int32_t absTenths = (tenths < 0) ? -tenths : tenths;
		lv_label_set_text_fmt(mCalibrationOffsetValue, "%s%d.%01d",
		                      (tenths < 0) ? "-" : "+",
		                      static_cast<int>(absTenths / 10),
		                      static_cast<int>(absTenths % 10));
		lv_obj_remove_flag(mCalibrationOffsetContainer, LV_OBJ_FLAG_HIDDEN);
	} else {
		lv_obj_add_flag(mCalibrationOffsetContainer, LV_OBJ_FLAG_HIDDEN);
	}
}

void DisplayManager::DrawDecontamination()
{
	if (mCurrentDecontaminationActive) {
		lv_obj_add_flag(mStatusBarContainer, LV_OBJ_FLAG_HIDDEN);
		lv_obj_remove_flag(mDecontaminationLabel, LV_OBJ_FLAG_HIDDEN);
		uint32_t mins = mCurrentDecontaminationElapsedSeconds / 60;
		uint32_t secs = mCurrentDecontaminationElapsedSeconds % 60;
		lv_label_set_text_fmt(mDecontaminationLabel, "DECON %u:%02u", mins, secs);
	} else {
		lv_obj_add_flag(mDecontaminationLabel, LV_OBJ_FLAG_HIDDEN);
		lv_obj_remove_flag(mStatusBarContainer, LV_OBJ_FLAG_HIDDEN);
	}
}

#endif /* CONFIG_DISPLAY */
