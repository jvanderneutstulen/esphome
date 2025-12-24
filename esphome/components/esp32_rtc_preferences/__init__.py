import logging

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_BUFFER_SIZE, CONF_ID, PLATFORM_ESP32

CODEOWNERS = ["@jvanderneutstulen"]
DOMAIN = "esp32_rtc_preferences"

DEPENDENCIES = [PLATFORM_ESP32]

_LOGGER = logging.getLogger(__name__)

esp32_rtc_preferences_ns = cg.esphome_ns.namespace(DOMAIN)
ESP32RTCPreferencesComponent = esp32_rtc_preferences_ns.class_(
    "ESP32RTCPreferencesComponent", cg.Component
)


CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(ESP32RTCPreferencesComponent),
        cv.Optional(CONF_BUFFER_SIZE, default=1024): cv.int_range(64, 8192),
    }
)


async def to_code(config):
    cg.add_define("USE_ESP32_PREFERENCES_RTC")
    cg.add_define("RTC_BUFFER_SIZE", config[CONF_BUFFER_SIZE])

    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
