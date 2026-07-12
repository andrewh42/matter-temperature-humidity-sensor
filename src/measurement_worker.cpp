/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include "measurement_worker.h"

#include "hdc302x_configuration.h"
#include "sht4x_configuration.h"
#include "io_worker.h"

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

#include <cerrno>

#ifdef CONFIG_DISPLAY
#include "display_manager.h"
#include "thread_status.h"
#endif

#ifndef CONFIG_DISPLAY
#include "board/board.h"
#endif

LOG_MODULE_REGISTER(measurement_worker, CONFIG_MEASUREMENT_WORKER_LOG_LEVEL);

namespace
{
#ifdef CONFIG_DT_HAS_TI_HDC302X_ENABLED
const device *sHdc302xSensorDev = DEVICE_DT_GET_ONE(ti_hdc302x);
#else
const device *sHdc302xSensorDev = nullptr;
#endif
#ifdef CONFIG_DT_HAS_SENSIRION_SHT4X_ENABLED
const device *sSht4xSensorDev = DEVICE_DT_GET_ONE(sensirion_sht4x);
#else
const device *sSht4xSensorDev = nullptr;
#endif
} /* namespace */

int MeasurementWorker::Init(PublishFn publish)
{
	mPublish = std::move(publish);

	mHdc302xSensor = Sensor(sHdc302xSensorDev, "HDC302x");
	mSht4xSensor   = Sensor(sSht4xSensorDev,   "SHT4x");

	if (mHdc302xSensor.IsAvailable()) {
		if (auto nistId = ReadHdc302xNistId()) {
			LOG_INF("HDC302x NIST ID: %s", nistId->c_str());
		}
	}
	if (mSht4xSensor.IsAvailable()) {
		if (auto serial = ReadSht4xSerialNumber()) {
			LOG_INF("SHT4x serial number: %s", serial->c_str());
		}
	}

	if (mHdc302xSensor.IsAvailable()) {
		mPrimarySensor   = &mHdc302xSensor;
		mSecondarySensor = mSht4xSensor.IsAvailable() ? &mSht4xSensor : nullptr;

		if (!ConfigureHdc302xAutomaticMeasurementMode(mHdc302xSensor.dev)) {
			return -EIO;
		}
#ifdef CONFIG_APP_HDC302X_MAINTENANCE_FEATURES
		mHumidityCalibrator.Init(mHdc302xSensor.dev);
		mDecontaminationController.Init(mHdc302xSensor.dev);
#endif
	} else if (mSht4xSensor.IsAvailable()) {
		mPrimarySensor   = &mSht4xSensor;
		mSecondarySensor = nullptr;
	} else {
		LOG_ERR("No sensor device available");
		return -ENODEV;
	}

	k_work_init(&mTickWork, TickHandler);
	k_work_init(&mTogglePrimaryWork, TogglePrimaryHandler);
#ifdef CONFIG_APP_HDC302X_MAINTENANCE_FEATURES
	k_work_init(&mCalibrationRequestWork, CalibrationRequestHandler);
	k_work_init(&mToggleDecontaminationWork, ToggleDecontaminationHandler);
#endif

#ifdef CONFIG_DISPLAY
	DisplayManager::Instance().SetPrimarySensorName(mPrimarySensor->name);
	DisplayManager::Instance().SetSecondarySensorPresent(mSecondarySensor != nullptr);
	UpdateDisplayedHumidityCalibrationOffset();
#endif

	return 0;
}

void MeasurementWorker::Start()
{
	k_timer_init(
		&mTickTimer,
		[](k_timer *) {
			k_work_submit_to_queue(&IoWorker::Instance().Queue(),
			                       &Instance().mTickWork);
		},
		nullptr);
	k_timer_start(&mTickTimer,
	              K_MSEC(kMeasurementsInitialMs),
	              K_MSEC(kMeasurementsIntervalMs));
}

void MeasurementWorker::RequestTogglePrimarySensor()
{
	k_work_submit_to_queue(&IoWorker::Instance().Queue(), &mTogglePrimaryWork);
}

#ifdef CONFIG_APP_HDC302X_MAINTENANCE_FEATURES
void MeasurementWorker::RequestHumidityCalibration()
{
	k_work_submit_to_queue(&IoWorker::Instance().Queue(),
	                       &mCalibrationRequestWork);
}

void MeasurementWorker::RequestToggleDecontamination()
{
	k_work_submit_to_queue(&IoWorker::Instance().Queue(),
	                       &mToggleDecontaminationWork);
}
#endif

void MeasurementWorker::TickHandler(k_work *)
{
	Instance().Tick();
}

void MeasurementWorker::TogglePrimaryHandler(k_work * /* work */)
{
	Instance().TogglePrimary();
}

#ifdef CONFIG_APP_HDC302X_MAINTENANCE_FEATURES
void MeasurementWorker::CalibrationRequestHandler(k_work * /* work */)
{
	Instance().mCalibrationRequested = true;
	LOG_INF("Humidity calibration requested (will apply on next measurement tick)");
}

void MeasurementWorker::ToggleDecontaminationHandler(k_work * /* work */)
{
	Instance().ToggleDecontamination();
}

void MeasurementWorker::ToggleDecontamination()
{
	if (mDecontaminationController.Active()) {
		mDecontaminationController.Stop();
		return;
	}
	if (mPrimarySensor != &mHdc302xSensor || !mHdc302xSensor.IsAvailable()) {
		LOG_WRN("Decontamination requires HDC302x as primary sensor");
		return;
	}
	mDecontaminationController.Start();
}
#endif

void MeasurementWorker::Tick()
{
#ifdef CONFIG_APP_HDC302X_MAINTENANCE_FEATURES
	if (mDecontaminationController.Active()) {
		return;
	}
#endif

#ifndef CONFIG_DISPLAY
	Nrf::LEDWidget &clusterUpdateLED = Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED3);
	clusterUpdateLED.Set(true);
#endif

	auto primaryResult = mPrimarySensor->Read();
	if (!primaryResult) {
		LOG_ERR("Failed to read primary sensor data: %d", primaryResult.error());
#ifndef CONFIG_DISPLAY
		clusterUpdateLED.Set(false);
#endif
		return;
	}
	auto [primaryTemperature, primaryHumidity] = primaryResult.value();

	std::optional<uint16_t> secondaryHumidity;
	if (mSecondarySensor != nullptr) {
		auto secondaryResult = mSecondarySensor->Read();
		if (secondaryResult) {
			secondaryHumidity = secondaryResult.value().humidity;
		} else {
			LOG_WRN("Failed to read secondary sensor data: %d", secondaryResult.error());
		}
	}

#ifdef CONFIG_APP_HDC302X_MAINTENANCE_FEATURES
	if (mCalibrationRequested) {
		mCalibrationRequested = false;
		const bool sht4xPrimary = (mPrimarySensor == &mSht4xSensor);
		if (!sht4xPrimary || !mHdc302xSensor.IsAvailable()) {
			LOG_WRN("Humidity calibration requires SHT4x primary and HDC302x present; skipped");
		} else {
			if (mHumidityCalibrator.Apply(primaryHumidity, secondaryHumidity)) {
				mHdc302xSensor.humidityAverage.reset();
#ifdef CONFIG_DISPLAY
				UpdateDisplayedHumidityCalibrationOffset();
#endif
			}
		}
	}
#endif

	mPublish(primaryTemperature, primaryHumidity);

#ifdef CONFIG_DISPLAY
	auto [connected, lqi] = GetThreadConnectivity();
	LOG_DBG("Thread connectivity: %s, LQI: %d", connected ? "connected" : "not connected", lqi);
	DisplayManager::Instance().UpdateSignalStrength(connected, lqi);

	DisplayManager::Instance().UpdateMeasurements(primaryTemperature, primaryHumidity);
	if (mSecondarySensor != nullptr) {
		DisplayManager::Instance().SetSecondaryHumidity(secondaryHumidity);
	}

	DisplayManager::Instance().RefreshDisplay();
#else
	clusterUpdateLED.Set(false);
#endif
}

void MeasurementWorker::TogglePrimary()
{
#ifdef CONFIG_APP_HDC302X_MAINTENANCE_FEATURES
	if (mDecontaminationController.Active()) {
		LOG_WRN("Sensor toggle ignored during decontamination");
		return;
	}
#endif
	if (mSecondarySensor == nullptr) {
		LOG_WRN("Sensor toggle requires both sensors in device tree");
		return;
	}

	std::swap(mPrimarySensor, mSecondarySensor);

	LOG_INF("Primary sensor: %s -> %s", mSecondarySensor->name, mPrimarySensor->name);

#ifdef CONFIG_DISPLAY
	DisplayManager::Instance().SetPrimarySensorName(mPrimarySensor->name);
	PushHumidityCalibrationOffsetToDisplay();
#endif

	Tick();
}

#ifdef CONFIG_DISPLAY
void MeasurementWorker::UpdateDisplayedHumidityCalibrationOffset()
{
	mHdc302xHumidityOffsetHundredths.reset();
	if (mHdc302xSensor.IsAvailable()) {
		struct sensor_value offset = {};
		int rc = sensor_attr_get(mHdc302xSensor.dev, SENSOR_CHAN_HUMIDITY,
		                         SENSOR_ATTR_OFFSET, &offset);
		if (rc == 0) {
			mHdc302xHumidityOffsetHundredths =
				static_cast<int16_t>(offset.val1 * 100 + offset.val2 / 10000);
		} else {
			LOG_WRN("Failed to read HDC302x humidity offset attr: %d", rc);
		}
	}
	PushHumidityCalibrationOffsetToDisplay();
}

void MeasurementWorker::PushHumidityCalibrationOffsetToDisplay()
{
	std::optional<int16_t> displayValue;
	if (mPrimarySensor == &mHdc302xSensor) {
		displayValue = mHdc302xHumidityOffsetHundredths;
	}
	DisplayManager::Instance().SetHumidityCalibrationOffset(displayValue);
}
#endif
