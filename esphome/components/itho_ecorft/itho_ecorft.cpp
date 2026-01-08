#include "itho_ecorft.h"
#include "esphome/core/log.h"

namespace esphome {
namespace itho_ecorft {

static const char *const TAG = "fan.itho_ecorft";

fan::FanCall IthoEcoRftFan::join() {
  ESP_LOGD(TAG, "Joining");
  //(this->enable_ == nullptr) ? this->set_itho_ecorft_levels_(1.0f, 1.0f) :
  // this->set_itho_ecorft_levels_(1.0f, 1.0f, 1.0f);
  return this->make_call().set_state(false);
}

void IthoEcoRftFan::setup() {
  // Construct traits
  this->traits_ = fan::FanTraits(false, true, false, this->speed_count_);
  this->traits_.set_supported_preset_modes(this->preset_modes_);

  if (this->cc1101_ == nullptr) {
    this->mark_failed();
  }
  this->cc1101_->register_listener(this);
}

void IthoEcoRftFan::dump_config() { LOG_FAN("", "Itho EcoRft Fan", this); }

void IthoEcoRftFan::control(const fan::FanCall &call) {
  if (call.get_state().has_value())
    this->state = *call.get_state();
  if (call.get_speed().has_value())
    this->speed = *call.get_speed();
  this->set_preset_mode_(call.get_preset_mode());

  // this->write_state_();
  this->publish_state();
}

// void IthoEcoRftFan::write_state_() {
//   float speed = this->state ? static_cast<float>(this->speed) / static_cast<float>(this->speed_count_) : 0.0f;
//   if (speed == 0.0f) {  // off means idle
//     (this->enable_ == nullptr) ? this->set_itho_ecorft_levels_(speed, speed)
//                                : this->set_itho_ecorft_levels_(speed, speed, speed);
//   } else
//       (this->enable_ == nullptr) ? this->set_itho_ecorft_levels_(0.0f, speed)
//                                  : this->set_itho_ecorft_levels_(0.0f, 1.0f, speed);
//   }
// }
//

void IthoEcoRftFan::on_packet(const std::vector<uint8_t> &packet, float freq_offset, float rssi, uint8_t lqi) {
  ESP_LOGD("itho ", "packet %s rssi %.1f dBm lqi %u offset %.1f", format_hex(packet).c_str(), rssi, lqi, freq_offset);
}

}  // namespace itho_ecorft
}  // namespace esphome
