idf_component_register( 
    SRCS "main.cpp"
        "tasks/ButtonTask.cpp"
        "tasks/LedTask.cpp"
        "tasks/UartTask.cpp"
    INCLUDE_DIRS "."
    REQUIRES 
        # standrd components
        esp_driver_gpio 
        esp_driver_uart 
        esp_timer 
        led_strip 
        #custom components
        button
)



