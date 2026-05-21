#include <array>
#include <cstdint>
#include "driver/gpio.h"

#include "SSD.h"

const std::array<std::uint8_t, 10> SSD::s_digits_bitmap{
    0b11111100,
    0b01100000,
    0b11011010,
    0b11110010,
    0b01100110,
    0b10110110,
    0b10111110,
    0b11100000,
    0b11111110,
    0b11110110,
};

SSD::SSD(bool i_common_anode,
         gpio_num_t i_serial_data_pin,
         gpio_num_t i_clock_pin,
         gpio_num_t i_clear_pin) : m_common_anode(i_common_anode),
                                   m_serial_data_pin(i_serial_data_pin),
                                   m_clock_pin(i_clock_pin),
                                   m_clear_pin(i_clear_pin)
{
}

void SSD::Init()
{
    gpio_config_t io_conf = {};
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask =
        (1ULL << m_serial_data_pin) |
        (1ULL << m_clock_pin) |
        (1ULL << m_clear_pin);

    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE;

    ESP_ERROR_CHECK(gpio_config(&io_conf));

    Clear();
}

void SSD::Clear()
{
    gpio_set_level(m_clear_pin, 0);
    gpio_set_level(m_clear_pin, 1);

    if (!m_common_anode)
        return;
    SendBits(0b11111111);
}

void SSD::Put(std::uint8_t i_digit)
{
    if (i_digit >= s_digits_bitmap.size())
        return;
    

    std::uint8_t output_bits = s_digits_bitmap[i_digit];
    if (m_common_anode) // If anode is common a segment glow when LOW.
        output_bits = ~output_bits;

    SendBits(output_bits);
}

void SSD::SendBits(std::uint8_t i_bits)
{
    for (std::uint8_t i = 0; i < 8; ++i)
    {
        if (i_bits & 0b00000001)
            gpio_set_level(m_serial_data_pin, 1);
        else
            gpio_set_level(m_serial_data_pin, 0);

        gpio_set_level(m_clock_pin, 0);
        gpio_set_level(m_clock_pin, 1);
        i_bits >>= 1;
    }
    gpio_set_level(m_serial_data_pin, 0);
}
