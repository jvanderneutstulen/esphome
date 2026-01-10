#pragma once

#include <cinttypes>
#include <vector>

#include "esphome/core/helpers.h"

#include "itho_ecorft.h"

namespace esphome {
namespace itho_ecorft {

enum MessageType {
  REQUEST = 0b00,
  INFORM = 0b01,
  WRITE = 0b10,
  RESPONSE = 0b11,
};

enum class MessageOpcode : uint16_t {
  FAN_STATUS = 0x31d9,
};

struct __attribute__((packed)) IthoHeader {
  union {
    uint8_t raw;
    struct {
      uint8_t : 2;
      MessageType MESSAGE_TYPE : 2;
      uint8_t DEVICE_ID : 2;
      bool PARAM0 : 1;
      bool PARAM1 : 1;
    };
  };
};

class IthoMessageDecoder {
 public:
  // static IthoMessage *decode(std::vector<uint8_t> packet);
};

class IthoMessage : public Parented<IthoEcoRftFan> {
 public:
  static IthoMessage *decode(std::vector<uint8_t> packet, IthoEcoRftFan *parent);
  static uint8_t calc_checksum(std::vector<uint8_t> packet, uint8_t len);

  virtual void process_msg(){/* default ignore */};

  MessageOpcode get_opcode() { return opcode_; };

  void set_type(MessageType msg_type) { msg_type_ = msg_type; };
  void set_device_id0(uint32_t device_id) { device_id0_ = device_id; };
  void set_device_id1(uint32_t device_id) { device_id1_ = device_id; };
  void set_device_id2(uint32_t device_id) { device_id2_ = device_id; };
  void set_param0(uint8_t param) {
    param0_ = param;
    has_param0_ = true;
  };
  void set_param1(uint8_t param) {
    param1_ = param;
    has_param1_ = true;
  };

  // Message Fields
  //<HEADER> <addr0> <addr1> <addr2> <param0> <param1> <OPCODE> <LENGTH> <PAYLOAD> <CHECKSUM>
  //<  1   > <  3  > <  3  > <  3  > <  1   > <  1   > <  2   > <  1   > <length > <   1    >

 private:
  virtual void decode_payload(std::vector<uint8_t> payload) { payload_ = payload; };
  std::vector<uint8_t> payload_{};

 protected:
  MessageType msg_type_{};
  uint32_t device_id0_{};
  uint32_t device_id1_{};
  uint32_t device_id2_{};
  bool has_param0_{false};
  bool has_param1_{false};
  uint8_t param0_{};
  uint8_t param1_{};
  MessageOpcode opcode_{};
};

class IthoFanStatusMessage : public IthoMessage {
 public:
  IthoFanStatusMessage() { opcode_ = MessageOpcode::FAN_STATUS; };

  virtual void process_msg();

 private:
  void decode_payload(std::vector<uint8_t> payload) override;

 protected:
  uint8_t speed_percent_{0xff};
};

}  // namespace itho_ecorft
}  // namespace esphome
