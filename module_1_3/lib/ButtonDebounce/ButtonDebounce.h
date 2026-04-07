#pragma once
#include <Arduino.h>
#include <cstdint>

class ButtonDebounce
{
public:
    ButtonDebounce(uint8_t i_gpio_pin, bool i_pulled_down, bool i_use_built_in_res) : m_gpio_pin(i_gpio_pin),
                                                                                      m_pulled_down(i_pulled_down),
                                                                                      m_use_built_in_res(i_use_built_in_res)
    {
        m_prev_state = m_current_state = !m_pulled_down;
        m_prev_timestamp = m_current_timestamp = millis();
    }

    void Init()
    {
        uint8_t pin_mode = INPUT;
        if (m_use_built_in_res)
        {
            if (m_pulled_down)
                pin_mode = INPUT_PULLDOWN;
            else
                pin_mode = INPUT_PULLUP;
        }
        pinMode(m_gpio_pin, pin_mode);
    };

    bool WasPressed() const { return m_current_state && !m_prev_state; }
    bool WasReleased() const { return !m_current_state && m_prev_state; }

    bool IsPressed() const { return m_current_state; }
    bool IsPressedFor(uint16_t i_period) { return IsPressed() && (millis() - m_prev_timestamp >= i_period); };

    void Update()
    {
        uint32_t now = millis();
        bool raw = digitalRead(m_gpio_pin);

        if (raw != m_last_raw_state)
        {
            m_last_raw_state = raw;
            m_last_change_time = now;
        }

        if (now - m_last_change_time >= m_debounce_period)
        {
            bool new_state = (raw == m_pulled_down);

            if (new_state != m_current_state)
            {
                m_prev_state = m_current_state;
                m_current_state = new_state;
                m_prev_timestamp = now;
            }
            else
            {
                m_prev_state = m_current_state;
            }
        }

        m_current_timestamp = now;
    }

private:
    const uint8_t m_debounce_period = 20;
    const uint8_t m_gpio_pin;
    const bool m_pulled_down;
    const bool m_use_built_in_res;
    bool m_prev_state;
    bool m_current_state;
    uint32_t m_prev_timestamp;
    uint32_t m_current_timestamp;
    bool m_last_raw_state;
    uint32_t m_last_change_time;
};
