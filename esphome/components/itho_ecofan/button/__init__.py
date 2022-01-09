import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import button
from esphome.const import (
    CONF_ID,
    DEVICE_CLASS_UPDATE,
    ENTITY_CATEGORY_CONFIG,
)
from .. import IthoEcoFanComponent, itho_ecofan_ns, CONF_ITHO_ECOFAN_ID

DEPENDENCIES = ["itho_ecofan"]

IthoEcoFanJoinButton = itho_ecofan_ns.class_(
    "IthoEcoFanJoinButton", button.Button, cg.Component
)

CONFIG_SCHEMA = (
    button.button_schema(
        device_class=DEVICE_CLASS_UPDATE, entity_category=ENTITY_CATEGORY_CONFIG
    )
    .extend(
        {
            cv.GenerateID(): cv.declare_id(IthoEcoFanJoinButton),
            cv.GenerateID(CONF_ITHO_ECOFAN_ID): cv.use_id(IthoEcoFanComponent),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_ITHO_ECOFAN_ID])
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await button.register_button(var, config)
    cg.add(var.set_parent(parent))
