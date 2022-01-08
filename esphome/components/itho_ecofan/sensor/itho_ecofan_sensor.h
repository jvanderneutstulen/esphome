#pragma once

#include "esphome/components/itho_ecofan/itho_ecofan.h"
#include "esphome/components/sensor/sensor.h"

namespace esphome {
namespace itho_ecofan {

class IthoEcoFanSensor : public sensor::Sensor, public Component {
 public:
  void setup() override;
  void set_parent(IthoEcoFanComponent *parent) { parent_ = parent; }

  void dump_config() override;

 protected:
  void update_from_parent_();
  IthoEcoFanComponent *parent_;
};

}  // namespace itho_ecofan
}  // namespace esphome
