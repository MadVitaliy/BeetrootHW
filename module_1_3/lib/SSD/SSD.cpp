#include <Arduino.h>

#include <array>
#include <cstdint>
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

SSD::SSD(bool i_common_anode, std::uint8_t i_serial_data_pin,
         std::uint8_t i_clock_pin,
         std::uint8_t i_clear_pin) : m_common_anode(i_common_anode),
                                     m_serial_data_pin(i_serial_data_pin),
                                     m_clock_pin(i_clock_pin),
                                     m_clear_pin(i_clear_pin),
                                     m_serial_data_pin_bitmask(1 << i_serial_data_pin),
                                     m_clock_pin_bitmask(1 << i_clock_pin),
                                     m_clear_pin_bitmask(1 << i_clear_pin)
{
}

void SSD::Init()
{
    pinMode(m_serial_data_pin, OUTPUT);
    pinMode(m_clock_pin, OUTPUT);
    pinMode(m_clear_pin, OUTPUT);

    GPIO.out_w1tc = m_serial_data_pin_bitmask;
    GPIO.out_w1tc = m_clock_pin_bitmask;

    Clear();
}

void SSD::Clear()
{
    GPIO.out_w1tc = m_clear_pin_bitmask;
    //delay(1);
    GPIO.out_w1ts = m_clear_pin_bitmask;

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
            GPIO.out_w1ts = m_serial_data_pin_bitmask;
        else
            GPIO.out_w1tc = m_serial_data_pin_bitmask;

        GPIO.out_w1tc = m_clock_pin_bitmask;
        GPIO.out_w1ts = m_clock_pin_bitmask;
        i_bits >>= 1;
    }
    GPIO.out_w1tc = m_serial_data_pin_bitmask;
}
