#include "Button.h"
#include "esp_timer.h"

uint32_t millis()
{
    return esp_timer_get_time() / 1000;
}

void Button::Init()
{
    gpio_config_t io_conf = {};

    io_conf.pin_bit_mask = (1ULL << m_gpio_pin);                                                               // Select GPIO 2
    io_conf.mode = GPIO_MODE_INPUT;                                                                            // Set as output
    io_conf.pull_up_en = m_use_built_in_res && !m_pulled_down ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE;      // Disable pull-up
    io_conf.pull_down_en = m_use_built_in_res && m_pulled_down ? GPIO_PULLDOWN_ENABLE : GPIO_PULLDOWN_DISABLE; // Disable pull-up
    io_conf.intr_type = GPIO_INTR_DISABLE;                                                                     // Disable interrupts

    gpio_config(&io_conf);
    m_last_update_timestamp = millis();
    m_debounce_start_timestamp = m_last_update_timestamp;
    m_press_timestamp = m_last_update_timestamp;
};

bool Button::GetMomentumButtonState()
{
    return bool(gpio_get_level(m_gpio_pin)) == m_pulled_down;
}

bool Button::IsPressedFor(uint16_t i_period, bool i_cancel)
{
    if (m_disabled)
        return false;
    if (!IsPressed() || m_last_update_timestamp - m_press_timestamp < i_period)
        return false;
    if (i_cancel)
        m_disabled = true;
    return true;
};

void Button::Update()
{
    m_last_update_timestamp = millis();
    const bool raw = GetMomentumButtonState();
    // Serial.println(raw);
    if (m_in_debounce)
    {
        if (m_last_update_timestamp - m_debounce_start_timestamp < m_debounce_period)
            return;

        m_in_debounce = false;

        if (raw != m_current_state)
        {
            // 2. Only m_current_state changes here
            m_current_state = raw;
            m_changed = true;
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
        m_changed = false;
    }
}
