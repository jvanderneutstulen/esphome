import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import (
    CONF_ID,
    STATE_CLASS_MEASUREMENT,
    UNIT_PERCENT,
    ICON_GAUGE,
)
from .. import IthoEcoFanComponent, itho_ecofan_ns, CONF_ITHO_ECOFAN_ID

IthoEcoFanSensor = itho_ecofan_ns.class_(
    "IthoEcoFanSensor", sensor.Sensor, cg.Component
)

CONFIG_SCHEMA = (
    sensor.sensor_schema(
        unit_of_measurement=UNIT_PERCENT,
        icon=ICON_GAUGE,
        accuracy_decimals=0,
        state_class=STATE_CLASS_MEASUREMENT,
    )
    .extend(
        {
            cv.GenerateID(): cv.declare_id(IthoEcoFanSensor),
            cv.GenerateID(CONF_ITHO_ECOFAN_ID): cv.use_id(IthoEcoFanComponent),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_ITHO_ECOFAN_ID])
    var = cg.new_Pvariable(config[CONF_ID])
    await sensor.register_sensor(var, config)
    await cg.register_component(var, config)

    cg.add(var.set_parent(parent))
