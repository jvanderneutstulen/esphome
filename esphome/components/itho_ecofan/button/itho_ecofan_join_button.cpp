#include "itho_ecofan_join_button.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

namespace esphome {
namespace itho_ecofan {

static const char *const TAG = "itho_ecofan.join_button";

void IthoEcoFanJoinButton::press_action() {
  ESP_LOGD(TAG, "Join button pressed!");

  this->parent_->join();
}

void IthoEcoFanJoinButton::dump_config() {
  LOG_BUTTON("", "Join Button", this);
}

}  // namespace itho_ecofan
}  // namespace esphome
