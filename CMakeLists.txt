#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr HINTS $ENV{ZEPHYR_BASE})

project(temperature_sensor)

# Enable GNU STD support.
include(${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/config/nrfconnect/app/enable-gnu-std.cmake)

# NORDIC SDK APP START

# Include all source files that are located in the Matter common directory.
include(${ZEPHYR_NRF_MODULE_DIR}/samples/matter/common/cmake/source_common.cmake)

# Include Data Model utils
include(${ZEPHYR_NRF_MODULE_DIR}/samples/matter/common/cmake/data_model.cmake)

include(${ZEPHYR_NRF_MODULE_DIR}/samples/matter/common/cmake/zap_helpers.cmake)
ncs_get_zap_parent_dir(ZAP_PARENT_DIR)

target_include_directories(app PRIVATE
  src
  third_party
  ${ZAP_PARENT_DIR}
)

target_sources(app PRIVATE
  src/app_task.cpp
  src/hdc302x_configuration.cpp
  src/main.cpp
  src/matter_reporter.cpp
  src/sensor.cpp
)

target_sources_ifdef(CONFIG_APP_HDC302X_MAINTENANCE_FEATURES app PRIVATE
  src/hdc302x_decontamination_controller.cpp
  src/hdc302x_humidity_calibrator.cpp
)

target_sources_ifdef(CONFIG_DISPLAY app PRIVATE
  src/display_manager.cpp
  src/thread_status.cpp
  src/fonts/lv_font_splinesans_medium_20.c
  src/fonts/lv_font_splinesans_bold_72.c
  src/fonts/lv_font_phosphor_32.c
)

if(CONFIG_DISPLAY)
  zephyr_compile_definitions(
    "LV_FONT_CUSTOM_DECLARE=LV_FONT_DECLARE(lv_font_splinesans_medium_20)"
    "LV_FONT_DEFAULT=&lv_font_splinesans_medium_20"
  )
endif()

ncs_configure_data_model()
# NORDIC SDK APP END
