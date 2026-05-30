/*
 * SPDX-License-Identifier: LicenseRef-Apache-2.0
 */

#include "matter_reporter.h"

#include <app-common/zap-generated/attributes/Accessors.h>
#include <lib/core/CHIPError.h>

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

using namespace ::chip;
using namespace ::chip::app;

namespace {
/* Matter spec invalid-value sentinels for the measurement clusters. */
constexpr int16_t  kTemperatureInvalidValue = 0x8000;
constexpr uint16_t kHumidityInvalidValue    = 0xFFFF;

template <auto GetFn, typename T>
CHIP_ERROR LoadAttributeBound(chip::EndpointId endpoint, const char *what, T &out)
{
	DataModel::Nullable<T> value;
	Protocols::InteractionModel::Status status = GetFn(endpoint, value);
	if (status != Protocols::InteractionModel::Status::Success || value.IsNull()) {
		LOG_ERR("Failed to get %s %x", what, to_underlying(status));
		return CHIP_ERROR_INCORRECT_STATE;
	}
	out = value.Value();
	return CHIP_NO_ERROR;
}
} /* namespace */

CHIP_ERROR MatterReporter::LoadValidityRanges()
{
	namespace TempAttr = Clusters::TemperatureMeasurement::Attributes;
	namespace HumAttr  = Clusters::RelativeHumidityMeasurement::Attributes;

	ReturnErrorOnFailure(LoadAttributeBound<TempAttr::MinMeasuredValue::Get>(
		kTemperatureEndpointId, "temperature measurement min value", mTemperatureMin));
	ReturnErrorOnFailure(LoadAttributeBound<TempAttr::MaxMeasuredValue::Get>(
		kTemperatureEndpointId, "temperature measurement max value", mTemperatureMax));
	ReturnErrorOnFailure(LoadAttributeBound<HumAttr::MinMeasuredValue::Get>(
		kHumidityEndpointId, "humidity measurement min value", mHumidityMin));
	ReturnErrorOnFailure(LoadAttributeBound<HumAttr::MaxMeasuredValue::Get>(
		kHumidityEndpointId, "humidity measurement max value", mHumidityMax));

	return CHIP_NO_ERROR;
}

void MatterReporter::Publish(std::optional<int16_t>  temperatureHundredths,
                             std::optional<uint16_t> humidityHundredths)
{
	int16_t temperature = kTemperatureInvalidValue;
	if (temperatureHundredths &&
	    *temperatureHundredths >= mTemperatureMin &&
	    *temperatureHundredths <= mTemperatureMax) {
		temperature = *temperatureHundredths;
	}

	Protocols::InteractionModel::Status status =
		Clusters::TemperatureMeasurement::Attributes::MeasuredValue::Set(
			kTemperatureEndpointId, temperature);
	if (status != Protocols::InteractionModel::Status::Success) {
		LOG_ERR("Updating temperature measurement %x", to_underlying(status));
	}

	uint16_t humidity = kHumidityInvalidValue;
	if (humidityHundredths &&
	    *humidityHundredths >= mHumidityMin &&
	    *humidityHundredths <= mHumidityMax) {
		humidity = *humidityHundredths;
	}

	status = Clusters::RelativeHumidityMeasurement::Attributes::MeasuredValue::Set(
		kHumidityEndpointId, humidity);
	if (status != Protocols::InteractionModel::Status::Success) {
		LOG_ERR("Updating relative humidity measurement %x", to_underlying(status));
	}
}
