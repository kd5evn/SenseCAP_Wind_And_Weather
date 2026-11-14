# Design summary

Condensed from the full design document (maintained as a Claude doc, with wireframes):
https://claude.ai/artifact/SCs7ERFY7cZz4poYs2hLvT

## Goal
Wind and weather display for a SenseCAP Indicator D1 Pro (ESP32-S3, 480x480 touch). Wind pages in the style of iKitesurf for up to 8 locations, with min / max / average / peak gust / direction and lull, a 6 h observed graph plus forecast continuation, and an Apple-Weather-style page for up to 8 locations. Optional split screen (wind and weather for one location). First spots: Hood River Waterfront (OR) and The Hatchery near Bingen (WA).

## Decisions
- ESP-IDF 5.1 + LVGL. **M0 uses LVGL 8.3.1 as bundled in Seeed's template** (the design doc says LVGL 9; moving to 9 is a later change, not needed for M0).
- Device calls public APIs directly (no relay; a relay is acceptable later). History in PSRAM, only settings in flash.
- Units default to mph and F.
- Observations: NWS, AviationWeather METAR, NDBC; Synoptic deferred but pluggable; Tempest only for an owned station. iKitesurf/iWindsurf stations and Windy's API are not usable (see the full doc, section 4).
- Forecast and weather: Open-Meteo (`best_match`, model is a per-location setting).
- Rideable-range band dropped. Lull trace added.

## Canonical sample
`wind_sample_t`, 12 bytes: Unix time (4), average, gust, lull (2 each, 0.1 m/s), direction (2). 720 samples per location.

## Milestones
M0 bring-up (this commit), M1 data spine, M2 observations, M3 forecast and setup, M4 weather page, M5 robustness, M6 release. Each has a gate; see the full doc.
