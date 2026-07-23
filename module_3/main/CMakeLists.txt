idf_component_register(SRCS "ButtonsArray.cpp" "main.cpp" "PowerManagement.cpp"
                        INCLUDE_DIRS "."
                        REQUIRES esp_driver_gpio esp_driver_ledc esp_adc esp_timer ssd joystick esp_driver_mcpwm
                       )

