.. highlight:: shell

Matter: Temperature and Humidity Sensor
#######################################

Hardware
********

This project is based on the Nordic nRF5340-DK development kit and the TI HDC302x temperature and humidity sensor.

The HDC302x is connected to the nRF5340-DK via I2C using the following pin configuration:

+----------------+---------------------+
| HDC302x Signal | nRF5340-DK Pin      |
+================+=====================+
| VCC            | VDD (header P1)     |
+----------------+---------------------+
| GND            | GND (header P1)     |
+----------------+---------------------+
| SDA            | P1.02 (header P4)   |
+----------------+---------------------+
| SCL            | P1.03 (header P4)   |
+----------------+---------------------+

The Adafruit HDC302x breakout board is a convenient option for connecting the sensor to the nRF5340-DK.

An alternative supported sensor is the SHT4x, which can be connected as follows:

+----------------+---------------------+
| SHT4x Signal   | nRF5340-DK Pin      |
+================+=====================+
| VCC            | VDD (header P1)     |
+----------------+---------------------+
| GND            | GND (header P1)     |
+----------------+---------------------+
| SDA            | P1.04 (header P4)   |
+----------------+---------------------+
| SCL            | P1.05 (header P4)   |
+----------------+---------------------+

When two sensors are attached, button 2 on the nRF5340-DK toggles between them.


Optional e-paper display
************************

The sample also supports an optional e-paper display (https://www.seeedstudio.com/ePaper-Breakout-Board-p-5804.html) connected to the nRF5340-DK via SPI
using the following pin configuration:

+----------------+----------------------+--------------+
| E-paper Signal  | nRF5340-DK Pin      | Ribbon cable |
+=================+=====================+==============+
| VCC             | VDD (header P1)     | Red          |
+-----------------+---------------------+--------------+
| GND             | GND (header P1)     | Brown        |
+-----------------+---------------------+--------------+
| BUSY            | P1.09 (header P4)   | Green        |
+-----------------+---------------------+--------------+
| RST             | P1.10 (header P4)   | Grey         |
+-----------------+---------------------+--------------+
| DC              | P1.11 (header P4)   | Blue         |
+-----------------+---------------------+--------------+
| CS              | P1.12 (header P4)   | Purple       |
+-----------------+---------------------+--------------+
| MOSI            | P1.13 (header P4)   | Orange       |
+-----------------+---------------------+--------------+
| SCK             | P1.15 (header P4)   | Yellow       |
+-----------------+---------------------+--------------+

Include the epaper.conf configuration file in the build to enable support for this display.


XIAO nRF54L15 wiring
********************

The Seeed XIAO nRF54L15 is an alternative supported target. The HDC302x is
connected via I2C using the following pin configuration:

+----------------+--------------------+
| HDC302x Signal | XIAO nRF54L15 Pin  |
+================+====================+
| VCC            | 3V3                |
+----------------+--------------------+
| GND            | GND                |
+----------------+--------------------+
| SDA            | D4 (P1.10)         |
+----------------+--------------------+
| SCL            | D5 (P1.11)         |
+----------------+--------------------+

The e-paper display is connected via SPI using the following pin configuration:

+-----------------+--------------------+--------------+
| E-paper Signal  | XIAO nRF54L15 Pin  | Ribbon cable |
+=================+====================+==============+
| VCC             | 3V3                | Red          |
+-----------------+--------------------+--------------+
| GND             | GND                | Brown        |
+-----------------+--------------------+--------------+
| BUSY            | D2 (P1.06)         | Green        |
+-----------------+--------------------+--------------+
| RST             | D0 (P1.04)         | Grey         |
+-----------------+--------------------+--------------+
| DC              | D3 (P1.07)         | Blue         |
+-----------------+--------------------+--------------+
| CS              | D1 (P1.05)         | Purple       |
+-----------------+--------------------+--------------+
| MOSI            | D10 (P2.02)        | Orange       |
+-----------------+--------------------+--------------+
| SCK             | D8 (P2.01)         | Yellow       |
+-----------------+--------------------+--------------+


How to build the software
*************************

Open this repo in Visual Studio Code and follow these steps:

1. Select the nRF Connect view using the sidebar on the left hand side.

2. Start a terminal configured for the nRF Connect SDK by clicking on `Open terminal` under the view's Welcome section.

3. Create a build configuration and build the temperature sensor software.

  For the Nordic nRF5340 DK (with HDC302x and e-paper display enabled)::

    west build -p -d build-nrf5340dk-hdc302x -b nrf5340dk/nrf5340/cpuapp -- -DEXTRA_CONF_FILE="epaper.conf" -DEXTRA_DTC_OVERLAY_FILE="app-hdc302x.overlay;epaper.overlay"

  For the Nordic nRF5340 DK (with HDC302x and SHT4x and e-paper display enabled)::

    west build -p -d build-nrf5340dk -b nrf5340dk/nrf5340/cpuapp -- -DEXTRA_CONF_FILE="epaper.conf" -DEXTRA_DTC_OVERLAY_FILE="app-hdc302x.overlay;app-sht4x.overlay;epaper.overlay"

  For the Nordic nRF5340 DK (with HDC302x and SHT4x and e-paper display enabled PLUS humidity calibration and decontamination features)::

    west build -p -d build-nrf5340dk -b nrf5340dk/nrf5340/cpuapp -- \
      -DEXTRA_CONF_FILE="epaper.conf" -DEXTRA_DTC_OVERLAY_FILE="app-hdc302x.overlay;app-sht4x.overlay;epaper.overlay" \
      -DCONFIG_APP_HDC302X_MAINTENANCE_FEATURES=y

  For the Seeed XIAO nRF54L15 (with HDC302x and e-paper display enabled)::

    west build -p -d build-xiao -b xiao_nrf54l15/nrf54l15/cpuapp -- \
      -DFILE_SUFFIX=internal \
      -DEXTRA_CONF_FILE="epaper.conf" -DEXTRA_DTC_OVERLAY_FILE="app-hdc302x-xiao.overlay;epaper-xiao.overlay"

  Add `xiao-external-antenna.overlay` as another EXTRA_DTC_OVERLAY_FILE if using the XIAO's external antenna.

4. Flash the software using the nRF Connect flash action.

  For the Nordic nRF5340 DK::

    west flash -d build-nrf5340dk --no-rebuild

  For the Seeed XIAO nRF54L15::

    west flash -d build-xiao


Production signing key
======================

The nRF54L15 builds sign the firmware (and DFU images) with your own Ed25519 key.
The private key is **not** committed to this repo (it is gitignored), so you must
generate it once before building::

  mkdir -p keys
  python3 /opt/nordic/ncs/v3.3.0/bootloader/mcuboot/scripts/imgtool.py \
      keygen -t ed25519 -k keys/mcuboot_ed25519_priv.pem

(Run from an nRF Connect SDK terminal so `imgtool`'s Python dependencies are available.)

**Back this key up to secure offline storage immediately.** If it is lost, no
future DFU image can ever be signed for already-deployed devices. The build picks
the key up automatically via `BOOT_SIGNATURE_KEY_FILE` in `Kconfig.sysbuild`.

By default the public verification key is **compiled into the MCUboot image**,
which works with the Seeed XIAO's onboard CMSIS-DAP probe and a plain `west flash`.

To instead store the public key in the SoC's hardware **Key Management Unit (KMU)**
(more hardened, supports key revocation), build with
`-DSB_CONFIG_MCUBOOT_SIGNATURE_USING_KMU=y` and flash with a SEGGER J-Link::

  west flash -d build-xiao --runner jlink --erase

KMU provisioning requires a J-Link (e.g. an nRF54L15 DK used as a debugger, or a
standalone J-Link); the XIAO's CMSIS-DAP probe cannot provision the KMU. The
private key and signing are identical to the compiled-in mode, so switching needs
no new key.


To modify the matter configuration:

1. Open an nrF terminal from VS Code.
2. Run west zap-gui
3. Make changes as appropriate, save (from zap menu), quit.
4. cd src/default_zap; west zap-generate
5. west zap-sync
6. Re-build (incremental build is fine: pristine not necessary).


How to add to Home
******************

1. Connect ethernet adapter to iPad and trust.
2. Disconnect iPad from Wi-Fi.
3. Connect the DK's VCOM1.
4. Open QR code by command-clicking on the connectedhome URL logged in the debug console (VCOM1)
5. Open the Home app on the iPad and choose Add Accessory from the "+" menu.
6. Scan QR code from step (4)
7. If adding the accessory fails, reboot the iPad. This worked last time!


Debugging
*********

The nRF5340-DK VCOM1 console can be used to run various CLI commands.

Read the temperature::

  sensor get hdc302x@44 ambient_temp


Testing
*******

Unit tests
==========

Unit tests run on the host and do not require any hardware. They use `Catch2 <https://github.com/catchorg/Catch2>`__ and are built
with CMake::

  cmake -S tests -B build-tests
  cmake --build build-tests
  ctest --test-dir build-tests
