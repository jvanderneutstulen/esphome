#pragma once

#include "esphome/components/itho_ecofan/itho_ecofan.h"
#include "esphome/components/button/button.h"

namespace esphome {
namespace itho_ecofan {

class IthoEcoFanComponent;

class IthoEcoFanJoinButton : public button::Button, public Component {
 public:
  void set_parent(IthoEcoFanComponent *parent) { this->parent_ = parent; }
  void dump_config() override;

 protected:
  void press_action() override;

  IthoEcoFanComponent *parent_;
};

}  // namespace itho_ecofan
}  // namespace esphome
