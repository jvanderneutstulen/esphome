#include "itho_ecofan_float_output.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

namespace esphome {
namespace itho_ecofan {

static const char *const TAG = "itho_ecofan_output";

void IthoEcoFanOutput::write_state(float state) {
  ESP_LOGD(TAG, "Current state %f, new state: %f", this->current_state_, state);
  if (std::isnan(this->current_state_) || roundf(state * 100) != roundf(this->current_state_ * 100)) {
	  this->current_state_ = state;
	  this->parent_->set_fan_speed(state);
  }
}

void IthoEcoFanOutput::setup() {
  ESP_LOGD(TAG, "Output setup");
}

void IthoEcoFanOutput::dump_config() {
  ESP_LOGCONFIG(TAG, "IthoEcoFan PWM:");
  LOG_FLOAT_OUTPUT(this);
}

}  // namespace itho_ecofan
}  // namespace esphome
