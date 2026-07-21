#pragma once

#include "driver/gpio.h"
#include "driver/pulse_cnt.h"

class Encoder
{
public:
    Encoder(gpio_num_t i_gpio_a, gpio_num_t i_gpio_b, gpio_num_t i_gpio_button) : m_gpio_a(i_gpio_a), m_gpio_b(i_gpio_b), m_gpio_button(i_gpio_button)
    {
    }

    void Init()
    {
        gpio_reset_pin(m_gpio_a);
        gpio_reset_pin(m_gpio_b);
        gpio_reset_pin(m_gpio_button);

        // 2. Initialize PCNT Unit
        pcnt_unit_config_t unit_config = {};
        unit_config.high_limit = 1024;
        unit_config.low_limit = -1024;
        unit_config.flags.accum_count = true;
        ESP_ERROR_CHECK(pcnt_new_unit(&unit_config, &mh_pcnt_unit));

        // Channel 0: Edge = A, Level = B
        pcnt_chan_config_t chan_config_0 = {};
        chan_config_0.edge_gpio_num = static_cast<int>(m_gpio_a);
        chan_config_0.level_gpio_num = static_cast<int>(m_gpio_b);
        ESP_ERROR_CHECK(pcnt_new_channel(mh_pcnt_unit, &chan_config_0, &mh_pcnt_channel_0));

        ESP_ERROR_CHECK(pcnt_channel_set_edge_action(mh_pcnt_channel_0,
                                                     PCNT_CHANNEL_EDGE_ACTION_DECREASE,
                                                     PCNT_CHANNEL_EDGE_ACTION_INCREASE));
        ESP_ERROR_CHECK(pcnt_channel_set_level_action(mh_pcnt_channel_0,
                                                      PCNT_CHANNEL_LEVEL_ACTION_KEEP,
                                                      PCNT_CHANNEL_LEVEL_ACTION_INVERSE));

        // Channel 1: Edge = B, Level = A
        pcnt_chan_config_t chan_config_1 = {};
        chan_config_1.edge_gpio_num = static_cast<int>(m_gpio_b);
        chan_config_1.level_gpio_num = static_cast<int>(m_gpio_a);
        ESP_ERROR_CHECK(pcnt_new_channel(mh_pcnt_unit, &chan_config_1, &mh_pcnt_channel_1));

        // NOTICE: Edge actions are reversed here (DECREASE, INCREASE)
        ESP_ERROR_CHECK(pcnt_channel_set_edge_action(mh_pcnt_channel_1,
                                                     PCNT_CHANNEL_EDGE_ACTION_INCREASE,
                                                     PCNT_CHANNEL_EDGE_ACTION_DECREASE));
        ESP_ERROR_CHECK(pcnt_channel_set_level_action(mh_pcnt_channel_1,
                                                      PCNT_CHANNEL_LEVEL_ACTION_KEEP,
                                                      PCNT_CHANNEL_LEVEL_ACTION_INVERSE));

        constexpr bool use_built_in_pullup_resistors = false;
        if (use_built_in_pullup_resistors) // TODO: make input parameters
        {
            gpio_pullup_en(m_gpio_a);
            gpio_pullup_en(m_gpio_b);
            gpio_pullup_en(m_gpio_button); // Pull up the button pin safely too
        }

        // 3. Glitch Filter & Start
        pcnt_glitch_filter_config_t filter_config = {};
        filter_config.max_glitch_ns = 500;
        ESP_ERROR_CHECK(pcnt_unit_set_glitch_filter(mh_pcnt_unit, &filter_config));

        ESP_ERROR_CHECK(pcnt_unit_enable(mh_pcnt_unit));
        ESP_ERROR_CHECK(pcnt_unit_clear_count(mh_pcnt_unit));
        ESP_ERROR_CHECK(pcnt_unit_start(mh_pcnt_unit));
        ESP_LOGI("ENCODER", "PCNT X4 Quadrature Mode Initialized");

        ESP_ERROR_CHECK(pcnt_unit_get_count(mh_pcnt_unit, &m_prev_counter_value));
        ESP_ERROR_CHECK(pcnt_unit_get_count(mh_pcnt_unit, &m_counter_value));
        Update();
    }

    bool Update()
    {
        m_prev_counter_value = m_counter_value;
        ESP_ERROR_CHECK(pcnt_unit_get_count(mh_pcnt_unit, &m_counter_value));
        ESP_LOGI("ENCODER", "Position: %d steps", m_counter_value);
        return false;
    }

    int GetDir() const
    {
        if (m_counter_value > m_prev_counter_value)
            return 1;
        else if (m_counter_value < m_prev_counter_value)
            return -1;
        return 0;
    }; // return -1 and 1 if stepped, 0 if not moved

private:
    gpio_num_t m_gpio_a;
    gpio_num_t m_gpio_b;
    gpio_num_t m_gpio_button;

    pcnt_unit_handle_t mh_pcnt_unit = nullptr;
    pcnt_channel_handle_t mh_pcnt_channel_0 = nullptr;
    pcnt_channel_handle_t mh_pcnt_channel_1 = nullptr;

    int m_counter_value = 0;
    int m_prev_counter_value = 0;
};
