#include "esphome/core/log.h"
#include "itho_messages.h"

namespace esphome {
namespace itho_ecorft {

static const char *const TAG = "fan.itho_ecorft.msg";

IthoMessage *IthoMessage::decode(std::vector<uint8_t> packet, IthoEcoRftFan *parent) {
  uint8_t i = 0;

  // FIXME: Add size checks
  if (packet.size() < 8) {
    // Packet too small
    return nullptr;
  }
  IthoHeader header;
  header.raw = packet[i++];
  MessageType msg_type = header.MESSAGE_TYPE;

  ESP_LOGVV(TAG, "MessageType %02x, DeviceId %02x, Param0 %d, Param1 %d", msg_type, header.DEVICE_ID, header.PARAM0,
            header.PARAM1);

  uint32_t device_id0{};
  uint32_t device_id1{};
  uint32_t device_id2{};

  switch (header.DEVICE_ID) {
    case 0b00:
      device_id0 = packet[i++] << 16 | packet[i++] << 8 | packet[i++];
      device_id1 = packet[i++] << 16 | packet[i++] << 8 | packet[i++];
      device_id2 = packet[i++] << 16 | packet[i++] << 8 | packet[i++];
      break;
    case 0b01:
      device_id2 = packet[i++] << 16 | packet[i++] << 8 | packet[i++];
      break;
    case 0b10:
      device_id0 = packet[i++] << 16 | packet[i++] << 8 | packet[i++];
      device_id2 = packet[i++] << 16 | packet[i++] << 8 | packet[i++];
      break;
    case 0b11:
      device_id0 = packet[i++] << 16 | packet[i++] << 8 | packet[i++];
      device_id1 = packet[i++] << 16 | packet[i++] << 8 | packet[i++];
      break;
  }
  ESP_LOGVV(TAG, "Device ID : %06x, %06x, %06x", device_id0, device_id1, device_id2);

  if (device_id0 == 0 && device_id1 == 0 && device_id2 == 0) {
    ESP_LOGD(TAG, "Invalid message, no device id");
    return nullptr;
  }

  uint8_t param0{};
  uint8_t param1{};

  if (header.PARAM0) {
    param0 = packet[i++];
  }

  if (header.PARAM1) {
    param1 = packet[i++];
  }

  MessageOpcode opcode = MessageOpcode(packet[i++] << 8 | packet[i++]);
  uint8_t length = packet[i++];

  ESP_LOGVV(TAG, "Opcode %04x, payload length %d", opcode, length);

  uint8_t crc = packet[i + length];
  uint8_t checksum = IthoMessage::calc_checksum(packet, i + length);

  if (crc != checksum) {
    ESP_LOGW(TAG, "CRC mismatch: found %02x, calculated %02x");
    return nullptr;
  }

  IthoMessage *msg{nullptr};
  switch (opcode) {
    case MessageOpcode::FAN_STATUS:
      msg = new IthoFanStatusMessage();
      break;
    case MessageOpcode::SPEED_COMMAND:
      msg = new IthoSpeedCommandMessage();
      break;
    default:
      ESP_LOGD(TAG, "Unknown opcode %04x", opcode);
      return nullptr;
  }

  msg->set_parent(parent);

  msg->set_type(msg_type);
  msg->set_device_id0(device_id0);
  msg->set_device_id1(device_id1);
  msg->set_device_id2(device_id2);
  msg->set_param0(param0);
  msg->set_param1(param1);

  std::vector<uint8_t> payload = std::vector<uint8_t>(packet.begin() + i, packet.begin() + i + length);
  msg->decode_payload(payload);

  return msg;
};

uint8_t IthoMessage::calc_checksum(std::vector<uint8_t> packet, uint8_t len) {
  uint8_t sum = 0;

  for (uint8_t i = 0; i < len; i++) {
    sum = (sum + packet[i]) & 0xff;
  }
  return 0 - sum;
}

std::vector<uint8_t> IthoMessage::encode(IthoEcoRftFan *parent) {
  if (parent != nullptr) {
    this->set_parent(parent);
  }
  this->init_msg();

  std::vector<uint8_t> payload = this->encode_payload();
  uint8_t length = payload.size();

  IthoHeader hdr{};
  hdr.MESSAGE_TYPE = this->msg_type_;
  hdr.PARAM0 = this->has_param0_;
  hdr.PARAM1 = this->has_param1_;

  std::vector<uint8_t> msg = {hdr.raw};

  uint8_t device_id_mask = (device_id2_ != 0) << 2 | (device_id1_ != 0) << 1 | (device_id0_ != 0);
  switch (device_id_mask) {
    case 0b111:
      // All
      hdr.DEVICE_ID = 0b00;
      msg.push_back(uint8_t(this->device_id0_ >> 16));
      msg.push_back(uint8_t(this->device_id0_ >> 8 & 0xff));
      msg.push_back(uint8_t(this->device_id0_ & 0xff));

      msg.push_back(uint8_t(this->device_id1_ >> 16));
      msg.push_back(uint8_t(this->device_id1_ >> 8 & 0xff));
      msg.push_back(uint8_t(this->device_id1_ & 0xff));

      msg.push_back(uint8_t(this->device_id2_ >> 16));
      msg.push_back(uint8_t(this->device_id2_ >> 8 & 0xff));
      msg.push_back(uint8_t(this->device_id2_ & 0xff));

      break;
    case 0b101:
      hdr.DEVICE_ID = 0b10;
      msg.push_back(uint8_t(this->device_id0_ >> 16));
      msg.push_back(uint8_t(this->device_id0_ >> 8 & 0xff));
      msg.push_back(uint8_t(this->device_id0_ & 0xff));

      msg.push_back(uint8_t(this->device_id2_ >> 16));
      msg.push_back(uint8_t(this->device_id2_ >> 8 & 0xff));
      msg.push_back(uint8_t(this->device_id2_ & 0xff));

      break;
    case 0b100:
      hdr.DEVICE_ID = 0b01;

      msg.push_back(uint8_t(this->device_id2_ >> 16));
      msg.push_back(uint8_t(this->device_id2_ >> 8 & 0xff));
      msg.push_back(uint8_t(this->device_id2_ & 0xff));
      break;
    case 0b011:
      hdr.DEVICE_ID = 0b11;

      msg.push_back(uint8_t(this->device_id0_ >> 16));
      msg.push_back(uint8_t(this->device_id0_ >> 8 & 0xff));
      msg.push_back(uint8_t(this->device_id0_ & 0xff));

      msg.push_back(uint8_t(this->device_id1_ >> 16));
      msg.push_back(uint8_t(this->device_id1_ >> 8 & 0xff));
      msg.push_back(uint8_t(this->device_id1_ & 0xff));

    default:
      ESP_LOGW(TAG, "Invalid combination of device ids");
      return std::vector<uint8_t>{};
  }

  msg[0] = hdr.raw;  // DEVICE_ID is now known
  if (has_param0_) {
    msg.push_back(param0_);
  }
  if (has_param1_) {
    msg.push_back(param1_);
  }
  msg.push_back(uint8_t(opcode_ >> 8));
  msg.push_back(uint8_t(opcode_ & 0xff));
  msg.push_back(length);
  msg.insert(msg.end(), payload.begin(), payload.end());
  uint8_t checksum = IthoMessage::calc_checksum(msg, msg.size());
  msg.push_back(checksum);

  return msg;
}

void IthoFanStatusMessage::decode_payload(std::vector<uint8_t> payload) {
  if (payload.size() < 3) {
    ESP_LOGW(TAG, "Fan speed status payload too small");
    return;
  }

  speed_percent_ = payload[2] >> 1;
}

void IthoFanStatusMessage::process_msg() {
  if (this->speed_percent_ == 0xff) {
    // No valid speed
    return;
  }
  ESP_LOGD(TAG, "Fan speed is %d%%", this->speed_percent_);

  int speed = 1;
  if (this->speed_percent_ < 25) {
    speed = 0;
  } else if (this->speed_percent_ > 75) {
    speed = 2;
  }
  this->parent_->state = speed > 0;
  if (speed > 0) {
    this->parent_->speed = speed;
  }
  this->parent_->publish_state();
}

void IthoSpeedCommandMessage::init_msg() {
  if (parent_ == nullptr) {
    return;
  }

  msg_type_ = MessageType::INFORM;
  device_id2_ = parent_->get_rf_address();
  this->set_param0(this->parent_->get_counter());
}

std::vector<uint8_t> IthoSpeedCommandMessage::encode_payload() {
  std::vector<uint8_t> payload{0x00, 0x00, 0x04};
  payload[1] = speed_;
  return payload;
}

}  // namespace itho_ecorft
}  // namespace esphome
