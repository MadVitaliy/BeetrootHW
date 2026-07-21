idf_component_register(SRCS "main.cpp"                        
                        INCLUDE_DIRS "."
                        REQUIRES esp_driver_gpio esp_driver_ledc esp_adc esp_timer esp_driver_mcpwm esp_driver_pcnt
                        joystick encoder
                       )

