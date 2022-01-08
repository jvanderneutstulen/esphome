#include "esphome/core/log.h"
#include "itho_ecofan.h"
#include "cc1101.h"
#include "itho_cc1101_protocol.h"


namespace esphome {
namespace itho_ecofan {

static const char *TAG = "itho_ecofan.component";

void ICACHE_RAM_ATTR IthoEcoFanComponentStore::gpio_intr(IthoEcoFanComponentStore *arg) {
  arg->data_available = true;
  arg->count = (arg->count + 1) % 0xFF;
}

void ICACHE_RAM_ATTR IthoEcoFanComponentStore::reset(IthoEcoFanComponentStore *arg) {
  arg->data_available = false;
}

std::string IthoEcoFanComponent::format_addr_(std::vector<uint8_t> addr) {
  std::string s;
  char buf[20];
  for (uint8_t a : addr) {
      sprintf(&buf[0], "%02X:", a);
      s += buf;
  }
  s.pop_back();
  return s;
}

void itho_ecofan::IthoEcoFanComponent::dump_config() {
  std::string s;

  // ESP_LOGCONFIG(TAG, "Itho EcoFan '%s'", this->fan_->get_name().c_str());
  if (this->rf_address_.size() > 0) {
    ESP_LOGCONFIG(TAG, "  RF Address: '%s'", this->format_addr_(this->rf_address_).c_str());
  }
  if (this->peer_rf_address_.size() > 0) {
    ESP_LOGCONFIG(TAG, "  RF Peer Address: '%s'", this->format_addr_(this->peer_rf_address_).c_str());
  }
  LOG_PIN("  CS Pin: ", this->cs_);
  LOG_PIN("  IRQ Pin: ", this->irq_);
  LOG_PIN("  RDY Pin: ", this->rdy_);

#ifdef ESPHOME_LOG_HAS_VERY_VERBOSE
  std::vector<uint8_t> config = this->cc1101_->read_burst_register(0x00, 47);
  for (uint8_t i = 0; i < config.size(); i++) {
    ESP_LOGCONFIG(TAG, "Config register [%02X] => [%02X]", i, config[i]);
  }
#endif
}

void IthoEcoFanComponent::setup() {

  if (this->cc1101_ == nullptr) {
    this->cc1101_ = new CC1101(this, this->rdy_, this->cs_);
  }

  if (this->itho_cc1101_ == nullptr) {
    this->itho_cc1101_ = new IthoCC1101(this->cc1101_, this->rf_address_);
  }

  this->spi_setup();

  if (!this->cc1101_->init()) {
    this->mark_failed();
    return;
  }

  // Setup module for Itho protocol
  this->itho_cc1101_->init_itho();

  // Enable interrupt on packet in RX FIFO
  this->irq_->setup();
  this->store_.data_available = false;
  this->store_.count = 0;
  this->store_.pin = this->irq_->to_isr();
  this->irq_->attach_interrupt(IthoEcoFanComponentStore::gpio_intr, &this->store_, gpio::INTERRUPT_RISING_EDGE);

  // Set CC1101 in receive mode
  this->itho_cc1101_->enable_receive_mode();
}
void IthoEcoFanComponent::loop() {

  if (this->store_.data_available) {

	IthoEcoFanComponentStore::reset(&this->store_);

    int16_t rssi = this->cc1101_->read_rssi();

    ESP_LOGD(TAG, "Data available in RX FIFO! (%02x) (%4d dBm)", this->store_.count, rssi);

    {
        uint8_t speed;

        if (this->itho_cc1101_->get_fan_speed(this->peer_rf_address_, &speed)) {

			// speed 0x1 -> C8
			uint8_t min_speed = 0x01;
			uint8_t max_speed = 0xC8;

			this->fan_speed_measured_ = ((speed - min_speed) * 1.0) / (max_speed - min_speed);

			this->itho_ecofan_callback_.call();
        }
    }

    this->itho_cc1101_->enable_receive_mode();
  }

}

float IthoEcoFanComponent::get_setup_priority() const { return setup_priority::DATA; }

void IthoEcoFanComponent::set_fan_speed(float value) {

	this->fan_speed_setting_ = value;

	if (std::isnan(this->fan_speed_measured_)) {
		ESP_LOGD(TAG, "Current speed unknown, ignore set");
		return;
	}

    std::string speed;

	if (value < 0.25) {
		speed = "low";
	} else if (value < 0.70) {
		speed = "medium";
	} else {
		speed = "high";
	}

    ESP_LOGD(TAG, "Setting speed: '%s'", speed.c_str());

    this->send_command_(speed);
}

//void IthoEcoFanComponent::join() {
//  ESP_LOGD(TAG, "Fan '%s': join() called", this->fan_->get_name().c_str());
//  this->send_command("join");
//}
//
void IthoEcoFanComponent::send_command_(std::string command) {

    this->itho_cc1101_->send_command(command);

    // After sending command switch back to receive mode
    this->itho_cc1101_->enable_receive_mode();

    this->set_timeout("send_command", 40, [this]() {
            this->schedule_send_packet_();
    });
}

void IthoEcoFanComponent::schedule_send_packet_() {

    uint8_t tries_left =  this->itho_cc1101_->send_packet();

    // After sending packet switch back to receive mode
    this->itho_cc1101_->enable_receive_mode();

    if (tries_left > 0) {
        this->set_timeout("send_command", 50, [this]() {
                this->schedule_send_packet_();
        });
    }
}

} // namespace itho_ecofan
}  // namespace esphome
