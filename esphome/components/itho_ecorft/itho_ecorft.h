#pragma once

#include "esphome/core/automation.h"
#include "esphome/components/cc1101/cc1101.h"
#include "esphome/components/fan/fan.h"

namespace esphome {
namespace itho_ecorft {

class IthoEcoRftFan : public Component, public fan::Fan, public cc1101::CC1101Listener {
 public:
  IthoEcoRftFan(int speed_count) : speed_count_(speed_count) {}

  void set_preset_modes(std::initializer_list<const char *> presets) { preset_modes_ = presets; }
  void set_rf_address(uint64_t rf_address) { rf_address_ = rf_address; }
  void set_peer_rf_address(uint64_t peer_rf_address) { peer_rf_address_ = peer_rf_address; }
  void set_cc1101(cc1101::CC1101Component *cc1101) { cc1101_ = cc1101; }

  void setup() override;
  void setup_cc1101();
  void dump_config() override;
  fan::FanTraits get_traits() override { return this->traits_; }

  fan::FanCall join();

  void on_packet(const std::vector<uint8_t> &packet, float freq_offset, float rssi, uint8_t lqi);
  void decode_packet(const std::vector<uint8_t> &packet);

 protected:
  int speed_count_{};
  fan::FanTraits traits_;
  std::vector<const char *> preset_modes_{};
  uint64_t rf_address_{};
  uint64_t peer_rf_address_{};

  cc1101::CC1101Component *cc1101_{nullptr};

  void control(const fan::FanCall &call) override;
  void write_state_();
};

template<typename... Ts> class JoinAction : public Action<Ts...> {
 public:
  explicit JoinAction(IthoEcoRftFan *parent) : parent_(parent) {}

  void play(const Ts &...x) override { this->parent_->join(); }

  IthoEcoRftFan *parent_;
};

}  // namespace itho_ecorft
}  // namespace esphome
