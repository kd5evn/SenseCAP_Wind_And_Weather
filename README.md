# SenseCAP Wind and Weather

Wind and weather display firmware for the Seeed Studio SenseCAP Indicator D1 Pro.
Design: [docs/DESIGN.md](docs/DESIGN.md) (full document with wireframes: https://claude.ai/artifact/SCs7ERFY7cZz4poYs2hLvT).

## Status: milestone M0 (bring-up)
Shows the wind page with **synthetic demo data** for two locations. There is no Wi-Fi, no real data, and no weather page yet.
It compiles cleanly with ESP-IDF 5.1.4. **It has not been run on a device yet**, so the first flash is also the first test: display, touch, swipe between locations, and tapping a chip to cycle the 1 h / 3 h / 6 h window.

## Flash (no toolchain needed)
Needs Python and `esptool` (`pip install esptool`). Plug the Indicator in over USB-C, then:

```
esptool.py --chip esp32s3 -b 460800 write_flash 0x0 release/sensecap_wind_M0_merged.bin
```

Add `-p <port>` (for example `COM5` or `/dev/ttyACM0`) if it is not auto-detected. The image is a merged 8 MB-layout image built for the D1 Pro; it overwrites the factory firmware, so keep a copy of Seeed's original if you want to go back.
Serial log at 115200 baud: `esptool` does not open it, use `idf.py monitor` or any serial terminal.

## Build from source
ESP-IDF 5.1.x (built and checked with 5.1.4), target esp32s3:

```
. $IDF_PATH/export.sh
idf.py set-target esp32s3
idf.py build
idf.py -p <port> flash monitor
```

Notes from building here: the new `idf-component-manager` 3.x breaks ESP-IDF 5.1; use `pip install "idf-component-manager~=1.5.0"` in the IDF Python environment if you see an `interface_version` error.

## Layout
- `main/` application: `ui/wind_page.c` (page and custom graph), `model/wind_model.c` (data model, stats, demo data)
- `components/` Seeed's board support, display, touch and I/O-expander drivers, vendored from [indicator-esp-idf-template](https://github.com/Seeed-Solution/indicator-esp-idf-template) (Apache-2.0), plus LVGL 8.3.1 with demos and examples removed
- `partitions.csv` 8 MB layout with two 3 MB OTA slots
- `sdkconfig.defaults` board = D1L/D1 Pro (with LoRa). For a base D1 or D1S, switch `CONFIG_LCD_BOARD_SENSECAP_INDICATOR_D1L` to `..._D1`.

## Next (M1)
Wi-Fi, SNTP and the request scheduler, then the NWS / METAR / NDBC providers (M2).
