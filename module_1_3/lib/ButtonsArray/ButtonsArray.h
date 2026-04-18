#pragma once
#include <Arduino.h>
#include <cstdint>
#include <optional>

class ButtonsArray
{
public:
    ButtonsArray(uint8_t i_analog_pin, uint8_t i_buttons_quantity) : m_in_pin(i_analog_pin),
                                                                     m_buttons_quantity(i_buttons_quantity)
    {
        // TODO: add algorithmic calculation of expected valus on buttons and ranges.
    }

    void Init();

    std::optional<uint8_t> ReadButton()
    {
        const uint16_t raw_value = analogRead(m_in_pin);
        // Serial.println(raw_value);
        for (uint8_t i = 0; i < m_buttons_quantity; ++i)
        {
            const auto i_button_pressed = raw_value >= m_safe_lowwer_limit[i] && raw_value <= m_safe_upper_limit[i];
            if (i_button_pressed)
                return i;
        }

        return {};
    }

private:
    const uint8_t m_in_pin; //
    const uint8_t m_buttons_quantity;
    uint16_t m_safe_upper_limit[4]{100, 2000, 2700, 3000}; // TODO: must have m_bottons_quantity length
    uint16_t m_safe_lowwer_limit[4]{0, 1850, 2550, 2875};
};
