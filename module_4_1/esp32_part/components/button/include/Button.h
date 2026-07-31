#pragma once

#include "driver/gpio.h"

class Button
{
public:
    Button(gpio_num_t i_gpio, bool i_pulled_down, bool i_use_built_in_res) : m_gpio_pin(i_gpio),
                                                                             m_pulled_down(i_pulled_down),
                                                                             m_use_built_in_res(i_use_built_in_res)
    {
    }

    void Init();

    bool WasPressed() const { return m_current_state && m_changed; }
    bool WasReleased() const { return !m_current_state && m_changed; }

    bool IsPressed() const { return m_current_state; }
    bool IsPressedFor(uint16_t i_period, bool i_cancel = false);
    void Update();

private:
    // Return true if button pressed, how button is connected does not matter.
    bool GetMomentumButtonState();

    const gpio_num_t m_gpio_pin;
    const uint8_t m_debounce_period = 20;
    const bool m_pulled_down;
    const bool m_use_built_in_res;

    bool m_current_state = false;
    bool m_changed = false;
    bool m_in_debounce = false;
    bool m_disabled = false;
    uint32_t m_debounce_start_timestamp;
    uint32_t m_last_update_timestamp;
    uint32_t m_press_timestamp;
};
