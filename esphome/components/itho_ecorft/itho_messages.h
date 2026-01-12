#pragma once

#include <cinttypes>
#include <vector>

#include "esphome/core/helpers.h"

#include "itho_ecorft.h"

namespace esphome {
namespace itho_ecorft {

class IthoEcoRftFan;

static const uint8_t HEADER_MESSAGE_TYPE_MASK = 0x30;
static const uint8_t MESSAGE_TYPE_REQUEST = 0x00;
static const uint8_t MESSAGE_TYPE_INFORM = 0x10;
static const uint8_t MESSAGE_TYPE_WRITE = 0x20;
static const uint8_t MESSAGE_TYPE_RESPONSE = 0x30;

static const uint8_t HEADER_ADDRESS_SPEC_MASK = 0x0c;
static const uint8_t ADDRESS_SPEC_0 = 0x00;  // 00 addr0 + addr1 + addr2
static const uint8_t ADDRESS_SPEC_1 = 0x04;  // 01 addr2
static const uint8_t ADDRESS_SPEC_2 = 0x08;  // 10 addr0 + addr2
static const uint8_t ADDRESS_SPEC_3 = 0x0c;  // 11 addr0 + addr1

// addr0, addr2 -> source
// addr1 -> destination

static const uint8_t HEADER_PARAM0_MASK = 0x02;
static const uint8_t HEADER_PARAM1_MASK = 0x01;

enum MessageType {
  REQUEST = 0b00,
  INFORM = 0b01,
  WRITE = 0b10,
  RESPONSE = 0b11,
};

enum MessageOpcode : uint16_t {
  FAN_STATUS = 0x31d9,
  // JOIN_COMMAND = 0x1fc9,
  SPEED_COMMAND = 0x22f1,
};

enum SpeedCommand : uint8_t {
  MIN = 0x01,
  LOW = 0x02,
  MEDIUM = 0x03,  // auto
  HIGH = 0x04,
  MAX = 0x05,
};

struct __attribute__((packed)) IthoHeader {
  union {
    uint8_t raw;
    struct {  // LSB first
      bool PARAM1 : 1;
      bool PARAM0 : 1;
      uint8_t DEVICE_ID : 2;
      MessageType MESSAGE_TYPE : 2;
      uint8_t : 2;
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
  std::vector<uint8_t> encode(IthoEcoRftFan *parent);

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

  virtual void set_src_addr(uint32_t src_addr){};
  // virtual void set_dst_addr(uint32_t dst_addr) {};

 private:
  virtual void decode_payload(std::vector<uint8_t> payload) { payload_ = payload; };
  virtual void init_msg(){};
  virtual std::vector<uint8_t> encode_payload() { return payload_; };

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
  uint8_t type_{};
  uint8_t addr_spec_{};
};

class IthoFanStatusMessage : public IthoMessage {
 public:
  IthoFanStatusMessage() { opcode_ = MessageOpcode::FAN_STATUS; };

  void process_msg() override;

 private:
  void decode_payload(std::vector<uint8_t> payload) override;

 protected:
  uint8_t speed_percent_{0xff};
};

#if 1
class IthoSpeedCommandMessage : public IthoMessage {
 public:
  IthoSpeedCommandMessage() {
    opcode_ = MessageOpcode::SPEED_COMMAND;
    addr_spec_ = ADDRESS_SPEC_1;
    has_param0_ = true;
  };

  void set_src_addr(uint32_t src_addr) override { device_id2_ = src_addr; };

  void set_speed(SpeedCommand speed) { speed_ = speed; };
  // void process_msg() override;

 private:
  void init_msg() override;
  std::vector<uint8_t> encode_payload() override;
  // void decode_payload(std::vector<uint8_t> payload) override;

 protected:
  SpeedCommand speed_{SpeedCommand::MEDIUM};
  // uint8_t speed_percent_{0xff};
};
#else
class IthoSpeedCommandMessage : public IthoMessage {
 public:
  IthoSpeedCommandMessage() { opcode_ = MessageOpcode::SPEED_COMMAND; };

  void set_src_addr(uint32_t src_addr) override { device_id2_ = src_addr; };

  void set_speed(SpeedCommand speed) { speed_ = speed; };

  // void process_msg() override;

 private:
  std::vector<uint8_t> encode_payload() override;
  // std::vector<uint8_t> encode_payload() override { return std::vector<uint8_t> {0x00}; };
  // void decode_payload(std::vector<uint8_t> payload) override;

 protected:
  SpeedCommand speed_{SpeedCommand::MEDIUM};
  // uint8_t speed_percent_{0xff};
};

#endif

}  // namespace itho_ecorft
}  // namespace esphome
