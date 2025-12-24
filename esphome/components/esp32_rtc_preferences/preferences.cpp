#ifdef USE_ESP32
#include "preferences.h"
#include "esphome/core/defines.h"
#include "esphome/core/log.h"
#include <memory>
#include <vector>

namespace esphome {

namespace esp32_rtc_preferences {

static const char *const TAG = "esp32_rtc_preferences";

// Allocate rtc_data in RTC Slow Memory. It will survive deep sleep.
RTC_DATA_ATTR uint8_t rtc_data[RTC_BUFFER_SIZE] = {};

struct RTCData {
  uint32_t key;
  std::unique_ptr<uint8_t[]> data;
  size_t len;

  void set_data(const uint8_t *src, size_t size) {
    this->data = std::make_unique<uint8_t[]>(size);
    memcpy(this->data.get(), src, size);
    this->len = size;
  }
};

static std::vector<RTCData> rtc_cached_data;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

class ESP32RTCPreferenceBackend : public ESPPreferenceBackend {
 public:
  uint32_t key;

  bool save(const uint8_t *data, size_t len) override {
    for (auto &obj : rtc_cached_data) {
      if (obj.key == this->key) {
        obj.set_data(data, len);
        ESP_LOGVV(TAG, "Save: updated key %08x, len %zu", this->key, len);
        return true;
      }
    }
    RTCData save{};
    save.key = this->key;
    save.set_data(data, len);
    rtc_cached_data.emplace_back(std::move(save));
    ESP_LOGVV(TAG, "Save: new key %08x, len %zu", this->key, len);
    return true;
  }

  bool load(uint8_t *data, size_t len) override {
    for (auto &obj : rtc_cached_data) {
      if (obj.key == this->key) {
        if (obj.len != len) {
          // size mismatch
          ESP_LOGV(TAG, "Load: key %08x data length mismatch (%zu != %zu)", this->key, obj.len, len);
          return false;
        }
        memcpy(data, obj.data.get(), len);
        ESP_LOGVV(TAG, "Load: found key %08x, len %zu", this->key, len);
        return true;
      }
    }
    ESP_LOGV(TAG, "Load: key %08x not found", this->key);
    return false;
  }
};

class ESP32RTCPreferences : public ESPPreferences {
 public:
  ESPPreferenceObject make_preference(size_t length, uint32_t type, bool in_flash) override {
    return this->make_preference(length, type);
  }

  ESPPreferenceObject make_preference(size_t length, uint32_t type) override {
    auto *pref = new ESP32RTCPreferenceBackend();  // NOLINT(cppcoreguidelines-owning-memory)
    pref->key = type;

    ESP_LOGVV(TAG, "Make pref object");
    return ESPPreferenceObject(pref);
  }

  ssize_t restore() {
    uint16_t written = 0;
    memcpy(&written, rtc_data, sizeof(written));

    if (written == 0) {
      return 0;
    }
    rtc_cached_data.clear();
    ESP_LOGVV(TAG, "Restoring %d entries", written);

    ssize_t index = sizeof(written);

    for (ssize_t i = 0; i < written && index < RTC_BUFFER_SIZE; i++) {
      RTCData stored{};

      memcpy(&stored.key, &rtc_data[index], sizeof(stored.key));
      index += sizeof(stored.key);

      uint16_t stored_len;
      memcpy(&stored_len, &rtc_data[index], sizeof(stored_len));
      index += sizeof(stored_len);
      stored.len = stored_len;

      if (stored.len > 0 && stored.len + index < RTC_BUFFER_SIZE) {
        stored.set_data(&rtc_data[index], stored.len);
        index += stored.len;
      } else {
        ESP_LOGV(TAG, "Invalid data length (%zu), skipping key %08x", stored.len, stored.key);
        continue;
      }
      ESP_LOGVV(TAG, "Restored key %08x: len %zu", stored.key, stored.len);

      rtc_cached_data.emplace_back(std::move(stored));
    }

    ESP_LOGV(TAG, "Restored %d/%d entries", rtc_cached_data.size(), written);

    return rtc_cached_data.size();
  }

  bool sync() override {
    uint16_t written = 0;
    ssize_t data_index = sizeof(written);

    ESP_LOGVV(TAG, "Syncing %d entries", rtc_cached_data.size());

    // Serialize data to allocated RTC memory buffer
    // Format: <num entries><key_1><len_1><data_1><key_2>...

    for (ssize_t i = 0; i < rtc_cached_data.size(); i++) {
      const auto &save = rtc_cached_data[i];
      ssize_t result = copy_into_rtc(data_index, save);

      if (result < 0) {
        ESP_LOGE(TAG, "Writing failed, not enough RTC memory");
        continue;
      }
      data_index = result;

      written++;
      memcpy(rtc_data, &written, sizeof(written));
    }

    ESP_LOGV(TAG, "Synced %d/%d entries", written, rtc_cached_data.size());

    return true;
  }

 protected:
  ssize_t copy_into_rtc(ssize_t index, const RTCData &to_save) {
    ESP_LOGVV(TAG, "Saving key %08x, len %d", to_save.key, to_save.len);

    if ((index + sizeof(struct RTCData) + to_save.len) >= RTC_BUFFER_SIZE) {
      return -1;
    }

    // key
    memcpy(&rtc_data[index], &to_save.key, sizeof(to_save.key));
    index += sizeof(to_save.key);

    // data length
    uint16_t save_len = to_save.len;
    memcpy(&rtc_data[index], &save_len, sizeof(save_len));
    index += sizeof(save_len);

    // data
    memcpy(&rtc_data[index], to_save.data.get(), to_save.len);
    index += to_save.len;

    return index;
  }

  bool reset() override {
    ESP_LOGD(TAG, "Erasing storage");
    rtc_cached_data.clear();
    memset(rtc_data, 0, RTC_BUFFER_SIZE);
    return true;
  }
};

void ESP32RTCPreferencesComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "ESP32 RTC Preferences:");
  ESP_LOGCONFIG(TAG, "  Size: %zu KB", RTC_BUFFER_SIZE / 1024);
}

void ESP32RTCPreferencesComponent::setup() {
  auto *prefs = new ESP32RTCPreferences();  // NOLINT(cppcoreguidelines-owning-memory)
  prefs->restore();
  rtc_preferences = prefs;

  set_interval("rtc_sync", 1000, []() { rtc_preferences->sync(); });
  this->disable_loop();
}

void ESP32RTCPreferencesComponent::on_shutdown() { rtc_preferences->sync(); }

float ESP32RTCPreferencesComponent::get_setup_priority() const { return setup_priority::IO; }

}  // namespace esp32_rtc_preferences

ESPPreferences *rtc_preferences;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

}  // namespace esphome

#endif
