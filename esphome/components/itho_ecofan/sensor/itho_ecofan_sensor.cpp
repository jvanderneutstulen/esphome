#include "itho_ecofan_sensor.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"

namespace esphome {
namespace itho_ecofan {

static const char *const TAG = "itho_ecofan.sensor";

void IthoEcoFanSensor::setup() {
  this->parent_->add_on_itho_ecofan_update_callback([this]() { this->update_from_parent_(); });
  this->update_from_parent_();
}
void IthoEcoFanSensor::update_from_parent_() {
  float value;
  value = this->parent_->get_fan_speed();
  this->publish_state(value * 100.0f);
}
void IthoEcoFanSensor::dump_config() { LOG_SENSOR("", "Itho EcoFan Sensor", this); }

}  // namespace itho_ecofan
}  // namespace esphome
