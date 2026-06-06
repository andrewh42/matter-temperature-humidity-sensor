/*
 * SPDX-License-Identifier: LicenseRef-Apache-2.0
 */

#pragma once

#include <optional>
#include <string>

/// Reads the 32-bit serial number from the SHT4x and returns it formatted as
/// an uppercase hex string with a "0x" prefix (e.g. "0x01234567"). Returns
/// std::nullopt if the sensor is not present in the device tree or if the
/// I2C transaction fails.
std::optional<std::string> ReadSht4xSerialNumber();
