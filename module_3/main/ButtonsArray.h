#pragma once
#include <cstdint>
#include <optional>

#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"

class ButtonsArray
{
public:
    ButtonsArray(adc_oneshot_unit_handle_t ip_adc_handle, adc_channel_t i_channel, uint8_t i_buttons_quantity)
        : mp_adc_handle(ip_adc_handle), m_channel(i_channel), m_buttons_quantity(i_buttons_quantity)
    {
        // TODO: add algorithmic calculation of expected valus on buttons and ranges.
    }

    void Init(adc_oneshot_unit_handle_t ip_adc_handle)
    {
        mp_adc_handle = ip_adc_handle;
        if (!mp_adc_handle)
            return;

        adc_oneshot_chan_cfg_t config = {
            .atten = ADC_ATTEN_DB_12,         // 0 to 3.3V full-scale range
            .bitwidth = ADC_BITWIDTH_DEFAULT, // Default max width for the chip (typically 12-bit)
        };
        ESP_ERROR_CHECK(adc_oneshot_config_channel(mp_adc_handle, m_channel, &config));
    }

    std::optional<uint8_t> ReadButton()
    {
        int raw_value;
        const esp_err_t ret_x = adc_oneshot_read(mp_adc_handle, m_channel, &raw_value);
        ESP_LOGI("", "raw: %d", raw_value);
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
    adc_oneshot_unit_handle_t mp_adc_handle = nullptr;
    adc_channel_t m_channel;

    const uint8_t m_buttons_quantity;
    uint16_t m_safe_upper_limit[4]{100, 2300, 2700, 3000}; // TODO: must have m_bottons_quantity length
    uint16_t m_safe_lowwer_limit[4]{0, 1750, 2400, 2700};
};
