#include "itho_ecorft.h"
#include "itho_messages.h"
#include "esphome/core/log.h"

namespace esphome {
namespace itho_ecorft {

static const char *const TAG = "fan.itho_ecorft";

const std::vector<uint8_t> ITHO_CC1101_HEADER{0x00, 0xb3, 0x2a, 0xab, 0x2a};
const uint8_t ITHO_CC1101_FOOTER_EVEN = 0xac;
const uint8_t ITHO_CC1101_FOOTER_ODD = 0xca;
const std::vector<uint8_t> ITHO_CC1101_FOOTER{ITHO_CC1101_FOOTER_EVEN, ITHO_CC1101_FOOTER_ODD};
//    static const uint8_t POSTAMBLE = 0xaa;

fan::FanCall IthoEcoRftFan::join() {
  ESP_LOGD(TAG, "Joining");
  // FIXME
  return this->make_call().set_state(false);
}

void IthoEcoRftFan::setup_cc1101() {
  if (this->cc1101_ == nullptr) {
    this->mark_failed();
  }
#if 0
  this->cc1101_->set_channel_spacing(200e3f);
  this->cc1101_->set_fsk_deviation(50781e3f);
  this->cc1101_->set_frequency(868.3e6f);
  this->cc1101_->set_modulation_type(cc1101::Modulation::MODULATION_2_FSK);
  this->cc1101_->set_symbol_rate(38400);
  this->cc1101_->set_carrier_sense_above_threshold(true);
  this->cc1101_->set_sync1(0xAB);
  this->cc1101_->set_sync0(0xFE);
  this->cc1101_->set_packet_mode(true);
  this->cc1101_->set_packet_length(64);
  this->cc1101_->reset();
#endif

  this->cc1101_->register_listener(this);
}

void IthoEcoRftFan::setup() {
  // Construct traits
  this->traits_ = fan::FanTraits(false, true, false, this->speed_count_);
  this->traits_.set_supported_preset_modes(this->preset_modes_);

  this->setup_cc1101();
}

void IthoEcoRftFan::dump_config() { LOG_FAN("", "Itho EcoRft Fan", this); }

void IthoEcoRftFan::control(const fan::FanCall &call) {
  if (call.get_state().has_value())
    this->state = *call.get_state();
  if (call.get_speed().has_value())
    this->speed = *call.get_speed();
  this->set_preset_mode_(call.get_preset_mode());

  this->write_state_();
  this->publish_state();
}

void IthoEcoRftFan::write_state_() {
  // float speed = this->state ? static_cast<float>(this->speed) / static_cast<float>(this->speed_count_) : 0.0f;
  int level = this->state ? this->speed : 0;
  ESP_LOGD(TAG, "Set speed level %d", level);
}

void IthoEcoRftFan::decode_packet(const std::vector<uint8_t> &packet) {
  auto msg_start = std::search(packet.begin(), packet.end(), ITHO_CC1101_HEADER.begin(), ITHO_CC1101_HEADER.end());

  if (msg_start == packet.end()) {
    // Not an Itho RF packet
    return;
  }
  msg_start += ITHO_CC1101_HEADER.size();  // Skip header

  auto msg_end = std::find_first_of(msg_start, packet.end(), ITHO_CC1101_FOOTER.begin(), ITHO_CC1101_FOOTER.end());

  if (msg_end == packet.end()) {
    ESP_LOGVV(TAG, "No footer found, dropping");
    return;
  }

  uint8_t msg_size = msg_end - msg_start;
  ESP_LOGV(TAG, "Found Itho message with length %d at offset %d", msg_size, (msg_start - packet.begin()));

  // Manchester decode
  std::vector<uint8_t> m1;
  m1.resize(msg_size);
  for (uint8_t i = 0; i < msg_size; ++i) {
    m1[i] = msg_start[i] xor 0x55;
  }

  uint8_t bits_size = msg_size * 4;  // 1 nibble per received byte
  std::vector<bool> bits_a, bits_b;
  bits_a.resize(bits_size, false);
  bits_b.resize(bits_size, false);
  for (uint8_t i = 0; i < bits_size; ++i) {
    bits_a[i] = (m1[i / 4] >> (6 - 2 * (i % 4))) & 0x01;
    bits_b[i] = (m1[i / 4] >> (6 - 2 * (i % 4))) & 0x02;
  }
  if (bits_a != bits_b) {
    ESP_LOGW(TAG, "Manchester decoding failed");
    return;
  }

  std::string b = "";
  for (auto elem : bits_a) {
    if (elem) {
      b += "1";
    } else {
      b += "0";
    }
  }

  // div into group of 5, drop last bit
  uint8_t nibbles = bits_size / 5;
  std::vector<uint8_t> msg;
  msg.resize(nibbles / 2, 0);
  for (uint8_t i = 0; i < nibbles; ++i) {
    uint8_t nibble = bits_a[5 * i + 0] << 3 | bits_a[5 * i + 1] << 2 | bits_a[5 * i + 2] << 1 | bits_a[5 * i + 3];
    msg[i / 2] = msg[i / 2] | (nibble << (4 * (1 - i % 2)));
  }

  const uint8_t reverse_nibble_lookup[16]{
      0x0, 0x8, 0x4, 0xc, 0x2, 0xa, 0x6, 0xe, 0x1, 0x9, 0x5, 0xd, 0x3, 0xb, 0x7, 0xf,
  };

  // Reverse each nibble
  for (uint8_t i = 0; i < msg.size(); ++i) {
    uint8_t n = msg[i];
    msg[i] = reverse_nibble_lookup[n >> 4] << 4 | reverse_nibble_lookup[n & 0x0f];
  }
  ESP_LOGVV(TAG, "Raw Itho message (%d) %s", msg.size(), format_hex(msg).c_str());

  auto *msg_obj = IthoMessage::decode(msg);
  if (msg_obj == nullptr) {
    return;
  }

  if (msg_obj->get_opcode() == MessageOpcode::FAN_STATUS) {
    IthoFanStatusMessage *m = static_cast<IthoFanStatusMessage *>(msg_obj);
    ESP_LOGD(TAG, "Fan speed is %d%%", m->get_speed_percentage());

    int speed = 1;
    if (m->get_speed_percentage() < 25) {
      speed = 0;
    } else if (m->get_speed_percentage() > 75) {
      speed = 2;
    }
    this->state = speed > 0;
    if (speed > 0) {
      this->speed = speed;
    }
    this->publish_state();
  }
}

void IthoEcoRftFan::on_packet(const std::vector<uint8_t> &packet, float freq_offset, float rssi, uint8_t lqi) {
  ESP_LOGVV(TAG, "packet %s rssi %.1f dBm lqi %u offset %.1f", format_hex(packet).c_str(), rssi, lqi, freq_offset);
  this->decode_packet(packet);
}

}  // namespace itho_ecorft
}  // namespace esphome
