#pragma once

#include "driver/gpio.h"
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

    Joystick(adc_oneshot_unit_handle_t ip_adc_handle, adc_channel_t i_x_channel, adc_channel_t i_y_channel);

    void Init(adc_oneshot_unit_handle_t ip_adc_handle);

    bool Update();

    Values GetRawValues() const;

    void SetOutputRanges(uint16_t x_min, uint16_t x_max, uint16_t y_min, uint16_t y_max);

    Values GetValues() const;

    int8_t GetXState() const;
    int8_t GetYState() const;

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

    static int8_t UpdateState(int8_t i_current_state, uint16_t i_new_value, const uint16_t *ip_ht_th, const uint16_t *ip_lt_th);
};
