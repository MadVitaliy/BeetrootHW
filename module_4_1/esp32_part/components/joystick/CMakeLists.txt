include($ENV{IDF_PATH}/tools/cmake/version.cmake)

set(srcs "src/Joystick.cpp")
set(public_requires)
list(APPEND public_requires "esp_driver_gpio esp_driver_ledc esp_common esp_adc esp_timer")

idf_component_register(SRCS ${srcs}
                       INCLUDE_DIRS "include"
                       REQUIRES ${public_requires})


# Why this is better than including sources as ussual/
# 
# ESP-IDF components provide:
# 
# * automatic include path handling
# * dependency propagation
# * isolated compilation
# * Kconfig support
# * unit testing support
# * reusable modules
# * easier scaling
# 
# This becomes very important once the project grows.
