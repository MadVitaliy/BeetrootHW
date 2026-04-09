
#include "ButtonDebounce.h"

void ButtonDebounce::Init()
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

    m_last_update_timestamp = millis();
    m_debounce_start_timestamp = m_last_update_timestamp;
    m_press_timestamp = m_last_update_timestamp;
};

bool ButtonDebounce::GetMomentumButtonState()
{
    return digitalRead(m_gpio_pin) == m_pulled_down;
}

bool ButtonDebounce::IsPressedFor(uint16_t i_period, bool i_cancel)
{
    if (m_disabled)
        return false;
    if (!IsPressed() || m_last_update_timestamp - m_press_timestamp < i_period)
        return false;
    if (i_cancel)
        m_disabled = true;
    return true;
};

void ButtonDebounce::Update()
{
    m_last_update_timestamp = millis();
    const bool raw = GetMomentumButtonState();
    m_prev_state = m_current_state;
    if (m_in_debounce)
    {
        if (m_last_update_timestamp - m_debounce_start_timestamp < m_debounce_period)
            return;

        m_in_debounce = false;

        if (raw != m_current_state)
        {
            m_prev_state = m_current_state;
            m_current_state = raw;
            m_disabled = false;
            if (m_current_state)
                m_press_timestamp = m_last_update_timestamp;
        }
    }
    else
    {
        if (raw != m_current_state)
        {
            m_debounce_start_timestamp = m_last_update_timestamp;
            m_in_debounce = true;
        }
    }
}
