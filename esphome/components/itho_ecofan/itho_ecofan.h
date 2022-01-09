#pragma once

#include "esphome/core/component.h"
#include "esphome/core/automation.h"
#include "esphome/components/spi/spi.h"

#include "cc1101.h"
#include "itho_cc1101.h"

namespace esphome {
namespace itho_ecofan {


struct IthoEcoFanComponentStore {
    volatile bool data_available;
    volatile uint8_t count;
    ISRInternalGPIOPin pin;

    static void reset(IthoEcoFanComponentStore *arg);
    static void gpio_intr(IthoEcoFanComponentStore *arg);
};


class IthoEcoFanComponent : public Component,
                            public spi::SPIDevice<spi::BIT_ORDER_MSB_FIRST, spi::CLOCK_POLARITY_LOW,
                                                  spi::CLOCK_PHASE_LEADING, spi::DATA_RATE_1MHZ> {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override;

  void set_irq_pin(InternalGPIOPin *irq) { irq_ = irq; }
  void set_rdy_pin(GPIOPin *rdy) { rdy_ = rdy; }
  void set_rf_address(uint64_t address) {
      this->rf_address_.resize(3, 0);
      for (uint8_t i = 0; i < 3; i++) {
          this->rf_address_[i] = (address >> (8 * (2 - i))) & 0xff;
      }
  }
  void set_peer_rf_address(uint64_t address) {
      this->peer_rf_address_.resize(3, 0);
      for (uint8_t i = 0; i < 3; i++) {
          this->peer_rf_address_[i] = (address >> (8 * (2 - i))) & 0xff;
      }
  }

  void join();

  void add_on_itho_ecofan_update_callback(std::function<void()> &&callback) {
    itho_ecofan_callback_.add(std::move(callback));
  }

  float get_fan_speed() const { return fan_speed_measured_; }
  void set_fan_speed(float value);

 protected:
  std::string format_addr_(std::vector<uint8_t> addr);

  GPIOPin *rdy_;
  InternalGPIOPin *irq_;

  std::vector<uint8_t> rf_address_;
  std::vector<uint8_t> peer_rf_address_;

  CC1101 *cc1101_{nullptr};
  IthoCC1101 *itho_cc1101_{nullptr};

  IthoEcoFanComponentStore store_;

  CallbackManager<void()> itho_ecofan_callback_;

  float fan_speed_measured_ = NAN;
  float fan_speed_setting_ = NAN;

  void send_command_(std::string command);
  void schedule_send_packet_();

  bool next_update_{true};
};


}  // namespace itho_ecofan
}  // namespace esphome

