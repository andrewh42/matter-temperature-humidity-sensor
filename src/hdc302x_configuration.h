/*
 * SPDX-License-Identifier: LicenseRef-Apache-2.0
 */

#pragma once

struct device;

/// Switches the HDC302x into automatic measurement mode at 0.5 Hz (one sample
/// every two seconds). Logs and returns false on failure.
bool ConfigureHdc302xAutomaticMeasurementMode(const device *hdc302xDevice);

/// Switches the HDC302x into manual-measurement mode: the sensor performs no
/// automatic sampling, so the host must trigger each reading. Required in
/// order to write the offset EEPROM. Logs and returns false on failure.
bool ConfigureHdc302xManualMeasurementMode(const device *hdc302xDevice);
