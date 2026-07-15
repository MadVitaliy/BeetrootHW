#pragma once

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_adc/adc_oneshot.h"
// #include "esp_common/adc_err.h"

namespace PrivateUtils
{
    template <typename TIn, typename TOut>
    TOut map(TIn in, TIn old_min, TIn old_max, TOut new_min, TOut new_max)
    {
        return new_min + static_cast<TOut>((in - old_min) * (new_max - new_min) / (old_max - old_min));
    }
}

class Joystick
{
public:
    struct Values
    {
        uint16_t x;
        uint16_t y;
    };

    Joystick(adc_oneshot_unit_handle_t ip_adc_handle, adc_channel_t i_x_channel, adc_channel_t i_y_channel)
        : mp_adc_handle(ip_adc_handle), m_x_channel(i_x_channel), m_y_channel(i_y_channel) {

          };

    void Init(adc_oneshot_unit_handle_t ip_adc_handle)
    {
        mp_adc_handle = ip_adc_handle;
        if (!mp_adc_handle)
            return;

        adc_oneshot_chan_cfg_t config = {
            .atten = ADC_ATTEN_DB_12,         // 0 to 3.3V full-scale range
            .bitwidth = ADC_BITWIDTH_DEFAULT, // Default max width for the chip (typically 12-bit)
        };
        ESP_ERROR_CHECK(adc_oneshot_config_channel(mp_adc_handle, m_x_channel, &config));
        ESP_ERROR_CHECK(adc_oneshot_config_channel(mp_adc_handle, m_y_channel, &config));

        constexpr uint8_t number_of_reading = 8;
        uint8_t reading_left = number_of_reading;
        uint32_t accamulated_x = 0,
                 accamulated_y = 0;

        while (reading_left)
        {
            ESP_ERROR_CHECK(adc_oneshot_read(mp_adc_handle, m_x_channel, raw_values));
            ESP_ERROR_CHECK(adc_oneshot_read(mp_adc_handle, m_y_channel, raw_values + 1));
            accamulated_x += raw_values[0];
            accamulated_y += raw_values[1];
            --reading_left;
        }
        const uint16_t centroid_x = accamulated_x / number_of_reading;
        const uint16_t centroid_y = accamulated_y / number_of_reading;

        constexpr uint16_t offset_from_center = 25;
        constexpr uint16_t hysterersys_width = 50;

        m_high_transition_x_th[0] = centroid_x + offset_from_center;
        m_high_transition_x_th[1] = m_high_transition_x_th[0] + hysterersys_width;
        m_low_transition_x_th[0] = centroid_x - offset_from_center;
        m_low_transition_x_th[1] = m_high_transition_x_th[0] - hysterersys_width;

        m_high_transition_y_th[0] = centroid_y + offset_from_center;
        m_high_transition_y_th[1] = m_high_transition_y_th[0] + hysterersys_width;
        m_low_transition_y_th[0] = centroid_y - offset_from_center;
        m_low_transition_y_th[1] = m_high_transition_y_th[0] - hysterersys_width;

        /* Let me explain what the hell is going on in the code above ^
        It's (no so) dummy initialization of two step hysteresys.
        I would like joystick axis to have 3 state: down -1, centered 0 and up 1.
        Thus hysteresys looks like
                      +--+----- < These are high transition thresholds
                      |  |
             +--+-----+--+      < And centroid is in the middle of here.
             |  |
        -----+--+               < These are low transition thresholds
        */
    };

    bool Update()
    {
        if (!mp_adc_handle)
            return false;

        const esp_err_t ret_x = adc_oneshot_read(mp_adc_handle, m_x_channel, raw_values);
        const esp_err_t ret_y = adc_oneshot_read(mp_adc_handle, m_y_channel, raw_values + 1);

        if (ret_x != ESP_OK || ret_y != ESP_OK)
            return false;

        m_state_x = UpdateState(m_state_x, raw_values[0], m_high_transition_x_th, m_low_transition_x_th);
        m_state_y = UpdateState(m_state_y, raw_values[1], m_high_transition_y_th, m_low_transition_y_th);

        return true;
    };

    Values GetRawValues() const
    {
        return {(uint16_t)raw_values[0], (uint16_t)raw_values[1]};
    };

    void SetOutputRanges(uint16_t x_min, uint16_t x_max, uint16_t y_min, uint16_t y_max)
    {
        m_user_x_range[0] = x_min;
        m_user_x_range[1] = x_max;
        m_user_y_range[0] = y_min;
        m_user_y_range[1] = y_max;
        m_convert_ranges = true;
    }

    Values GetValues() const
    {
        if (!m_convert_ranges)
            return GetRawValues();

        const uint16_t x = PrivateUtils::map<uint16_t, uint16_t>(raw_values[0], 0, 4095, m_user_x_range[0], m_user_x_range[1]);
        const uint16_t y = PrivateUtils::map<uint16_t, uint16_t>(raw_values[1], 0, 4095, m_user_y_range[0], m_user_y_range[1]);
        return {x, y};
    }

    int8_t GetXState() const { return m_state_x; }
    int8_t GetYState() const { return m_state_y; }

private:
    // using MapValues = ;

    adc_oneshot_unit_handle_t mp_adc_handle = nullptr;
    adc_channel_t m_x_channel;
    adc_channel_t m_y_channel;
    int raw_values[2];
    bool m_convert_ranges = false;
    uint16_t m_user_x_range[2];
    uint16_t m_user_y_range[2];
    uint16_t m_y_limits[2];

    int8_t m_state_x = 0;
    int8_t m_state_y = 0;
    uint16_t m_low_transition_x_th[2];
    uint16_t m_high_transition_x_th[2];
    uint16_t m_low_transition_y_th[2];
    uint16_t m_high_transition_y_th[2];
    // Button m_button;

    static int8_t UpdateState(int8_t i_current_state, uint16_t i_new_value, const uint16_t *ip_ht_th, const uint16_t *ip_lt_th)
    {
        switch (i_current_state)
        {
        case 0:
            // To drop into -1, it must fall below the lower threshold
            if (i_new_value < ip_lt_th[1])
            {
                i_current_state = -1;
            }
            // To jump into 1, it must rise above the upper threshold
            else if (i_new_value > ip_ht_th[1])
            {
                i_current_state = 1;
            }
            break;
        case -1:
            // To leave -1 and go to 0, the reading must cross the higher threshold
            if (i_new_value > ip_lt_th[0])
            {
                i_current_state = 0;
            }
            break;

        case 1:
            // To leave 1 and drop back to 0, it must fall below the lower threshold
            if (i_new_value < ip_ht_th[0])
            {
                i_current_state = 0;
            }
            break;
        }
        return i_current_state;
    }
};
