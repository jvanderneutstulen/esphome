import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import core, pins
from esphome.components import spi
from esphome.const import CONF_ID

CODEOWNERS = ["@jaspervanderneut"]
DEPENDENCIES = ["spi"]
MULTI_CONF = True

itho_ecofan_ns = cg.esphome_ns.namespace("itho_ecofan")
IthoEcoFanComponent = itho_ecofan_ns.class_(
    "IthoEcoFanComponent", cg.Component, spi.SPIDevice
)

CONF_ITHO_ECOFAN_ID = "itho_ecofan_id"

CONF_ITHO_IRQ_PIN = "irq_pin"
CONF_ITHO_RDY_PIN = "rdy_pin"
CONF_RF_ADDRESS = "rf_address"
CONF_PEER_RF_ADDRESS = "peer_rf_address"


def validate_rf_address(value):
    value = cv.string_strict(value)
    parts = value.split(":")
    if len(parts) != 3:
        raise cv.Invalid("RF Address must consist of 3 : (colon) separated parts")

    parts_int = []

    if any(len(part) != 2 for part in parts):
        raise cv.Invalid("RF Address must be format XX:XX:XX")

    for part in parts:
        parts_int.append(0)

    for part in parts:
        try:
            parts_int.append(int(part, 16))
        except ValueError:
            raise cv.Invalid(
                "RF Address parts must be hexadecimal values from 00 to FF"
            )

    return core.MACAddress(*parts_int)


def validate(config):
    if CONF_PEER_RF_ADDRESS in config and CONF_RF_ADDRESS in config:
        if config[CONF_PEER_RF_ADDRESS] == config[CONF_RF_ADDRESS]:
            raise cv.Invalid("RF address cannot be the same as peer RF address!")

    return config


CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(IthoEcoFanComponent),
            cv.Required(CONF_ITHO_IRQ_PIN): pins.gpio_input_pin_schema,
            cv.Required(CONF_ITHO_RDY_PIN): pins.gpio_input_pin_schema,
            cv.Optional(CONF_RF_ADDRESS): validate_rf_address,
            cv.Optional(CONF_PEER_RF_ADDRESS): validate_rf_address,
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(spi.spi_device_schema(cs_pin_required=True))
)

FINAL_VALIDATE_SCHEMA = spi.final_validate_device_schema(
    "itho_ecofan", require_miso=True, require_mosi=True
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])

    irq = await cg.gpio_pin_expression(config[CONF_ITHO_IRQ_PIN])
    cg.add(var.set_irq_pin(irq))

    rdy = await cg.gpio_pin_expression(config[CONF_ITHO_RDY_PIN])
    cg.add(var.set_rdy_pin(rdy))

    if CONF_RF_ADDRESS in config:
        cg.add(var.set_rf_address(config[CONF_RF_ADDRESS].as_hex))

    if CONF_PEER_RF_ADDRESS in config:
        cg.add(var.set_peer_rf_address(config[CONF_PEER_RF_ADDRESS].as_hex))

    await cg.register_component(var, config)
    await spi.register_spi_device(var, config)
