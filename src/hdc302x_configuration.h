/*
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <optional>
#include <string>

struct device;

/// Switches the HDC302x into automatic measurement mode at 0.5 Hz (one sample
/// every two seconds). Logs and returns false on failure.
bool ConfigureHdc302xAutomaticMeasurementMode(const device *hdc302xDevice);

/// Switches the HDC302x into manual-measurement mode: the sensor performs no
/// automatic sampling, so the host must trigger each reading. Required in
/// order to write the offset EEPROM. Logs and returns false on failure.
bool ConfigureHdc302xManualMeasurementMode(const device *hdc302xDevice);

/// Reads the 48-bit NIST ID (serial number) from the HDC302x and returns it
/// formatted as an uppercase hex string with a "0x" prefix (e.g.
/// "0x0123456789AB"). Returns std::nullopt if the sensor is not present in
/// the device tree or if the I2C transaction fails.
std::optional<std::string> ReadHdc302xNistId();
