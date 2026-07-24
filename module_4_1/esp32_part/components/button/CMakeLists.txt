include($ENV{IDF_PATH}/tools/cmake/version.cmake)

set(srcs "src/Button.cpp")
set(public_requires)
list(APPEND public_requires "esp_driver_gpio esp_common esp_timer")

idf_component_register(SRCS ${srcs}
                       INCLUDE_DIRS "include"
                       REQUIRES ${public_requires})
