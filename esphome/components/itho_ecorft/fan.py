from esphome import automation
from esphome.automation import maybe_simple_id
import esphome.codegen as cg
from esphome.components import cc1101, fan
from esphome.components.fan import validate_preset_modes
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_PRESET_MODES

SPEED_COUNT = 100

CODEOWNERS = ["@jvanderneutstulen"]

DEPENDENCIES = ["cc1101"]

itho_ecorft_ns = cg.esphome_ns.namespace("itho_ecorft")
IthoEcoRftFan = itho_ecorft_ns.class_("IthoEcoRftFan", cg.Component, fan.Fan)

# Actions
JoinAction = itho_ecorft_ns.class_("JoinAction", automation.Action)

CONF_CC1101_ID = "cc1101_id"
CONF_RF_ADDRESS = "rf_address"
CONF_PEER_RF_ADDRESS = "peer_rf_address"


def validate(config):
    if (
        CONF_PEER_RF_ADDRESS in config
        and config[CONF_PEER_RF_ADDRESS] == config[CONF_RF_ADDRESS]
    ):
        raise cv.Invalid("RF address cannot be the same as peer RF address!")

    return config


def _validate_rf_address(value):
    try:
        address = cv.mac_address(f"00:00:00:{value}")
    except cv.Invalid:
        raise cv.Invalid("RF Address must be hexadecimal values in format XX:XX:XX")
    return address


CONFIG_SCHEMA = (
    fan.fan_schema(IthoEcoRftFan)
    .extend(
        {
            cv.Required(CONF_CC1101_ID): cv.use_id(cc1101.CC1101Component),
            cv.Required(CONF_RF_ADDRESS): _validate_rf_address,
            cv.Optional(CONF_PEER_RF_ADDRESS): _validate_rf_address,
            cv.Optional(
                CONF_PRESET_MODES, default=["low", "medium", "high"]
            ): validate_preset_modes,
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)


ECOFAN_ACTION_SCHEMA = maybe_simple_id(
    {
        cv.GenerateID(): cv.use_id(IthoEcoRftFan),
    }
)


@automation.register_action("fan.itho_ecorft.join", JoinAction, ECOFAN_ACTION_SCHEMA)
async def fan_itho_ecorft_join_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(action_id, template_arg, paren)


async def to_code(config):
    var = await fan.new_fan(
        config,
        SPEED_COUNT,
    )
    await cg.register_component(var, config)

    cc1101_ = await cg.get_variable(config[CONF_CC1101_ID])
    cg.add(var.set_cc1101(cc1101_))

    cg.add(var.set_rf_address(config[CONF_RF_ADDRESS].as_hex))

    if CONF_PEER_RF_ADDRESS in config:
        cg.add(var.set_peer_rf_address(config[CONF_PEER_RF_ADDRESS].as_hex))

    if CONF_PRESET_MODES in config:
        cg.add(var.set_preset_modes(config[CONF_PRESET_MODES]))
