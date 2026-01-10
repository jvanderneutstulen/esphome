#include <span>
#include <ranges>
#include "itho_messages.h"
#include "esphome/core/log.h"

namespace esphome {
namespace itho_ecorft {

static const char *const TAG = "fan.itho_ecorft.msg";

IthoMessage *IthoMessage::decode(std::vector<uint8_t> packet) {
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
    default:
      ESP_LOGD(TAG, "Unknown opcode %04x", opcode);
      return nullptr;
  }

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

void IthoFanStatusMessage::decode_payload(std::vector<uint8_t> payload) {
  if (payload.size() < 3) {
    ESP_LOGW(TAG, "Fan speed status payload too small");
    return;
  }

  speed_percent_ = payload[2] >> 1;
}

}  // namespace itho_ecorft
}  // namespace esphome
