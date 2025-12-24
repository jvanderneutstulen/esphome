#pragma once

#ifdef USE_ESP32

#include "esphome/core/component.h"
#include "esphome/core/preferences.h"

namespace esphome {
namespace esp32_rtc_preferences {

// static constexpr size_t RTC_SIZE = RTC_BUFFER_SIZE;

class ESP32RTCPreferencesComponent : public Component {
  void dump_config() override;

  void setup() override;

  void on_shutdown() override;

  float get_setup_priority() const override;
};

// void setup_preferences();

}  // namespace esp32_rtc_preferences

extern ESPPreferences *rtc_preferences;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

}  // namespace esphome

#endif
