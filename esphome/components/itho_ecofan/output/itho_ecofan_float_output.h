#pragma once

#include "esphome/components/itho_ecofan/itho_ecofan.h"
#include "esphome/components/output/float_output.h"

namespace esphome {
namespace itho_ecofan {

class IthoEcoFanComponent;

class IthoEcoFanOutput : public output::FloatOutput, public Component {
 public:
  void set_parent(IthoEcoFanComponent *parent) { this->parent_ = parent; }
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::HARDWARE; }

 protected:
  void write_state(float state) override;

  float current_state_{NAN};

  IthoEcoFanComponent *parent_;
};

}  // namespace itho_ecofan
}  // namespace esphome
