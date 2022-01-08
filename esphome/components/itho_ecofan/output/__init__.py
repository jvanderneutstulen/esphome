import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import output
from esphome.const import CONF_ID
from .. import IthoEcoFanComponent, itho_ecofan_ns, CONF_ITHO_ECOFAN_ID

DEPENDENCIES = ["itho_ecofan"]

IthoEcoFanOutput = itho_ecofan_ns.class_(
    "IthoEcoFanOutput", output.FloatOutput, cg.Component
)

CONFIG_SCHEMA = output.FLOAT_OUTPUT_SCHEMA.extend(
    {
        cv.Required(CONF_ID): cv.declare_id(IthoEcoFanOutput),
        cv.GenerateID(CONF_ITHO_ECOFAN_ID): cv.use_id(IthoEcoFanComponent),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_ITHO_ECOFAN_ID])
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await output.register_output(var, config)
    cg.add(var.set_parent(parent))
