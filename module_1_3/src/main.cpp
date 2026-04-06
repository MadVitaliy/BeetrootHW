#include <Arduino.h>
#include "configs.h"
#include <array>
#include <cstdint>

class SSD
{
  // Has nothing in common with solid-state drive.
  // Stands for 7-segments display.
public:
  SSD(bool i_common_anode, std::uint8_t i_serial_data_pin,
      std::uint8_t i_clock_pin,
      std::uint8_t i_clear_pin) : m_common_anode(i_common_anode), m_serial_data_pin(i_serial_data_pin), m_clock_pin(i_clock_pin), m_clear_pin(i_clear_pin) {}

  void Init()
  {
    pinMode(m_serial_data_pin, OUTPUT);
    pinMode(m_clock_pin, OUTPUT);
    pinMode(m_clear_pin, OUTPUT);

    digitalWrite(m_serial_data_pin, LOW);
    digitalWrite(G_SHR_CLOCK_PIN, LOW);

    Clear();
  }

  void Clear()
  {
    digitalWrite(m_clear_pin, LOW);
    delay(1);
    digitalWrite(m_clear_pin, HIGH);
    if (!m_common_anode)
      return;
    SendBits(0b11111111);
  }

  void put(std::uint8_t i_digit)
  {
    if (i_digit >= s_digits_bitmap.size())
      return;

    std::uint8_t output_bits = s_digits_bitmap[i_digit];
    if (m_common_anode) // If anode is common a segment glow when LOW.
      output_bits = ~output_bits;

    SendBits(output_bits);
  }

private:
  void SendBits(std::uint8_t i_bits)
  {
    for (std::uint8_t i = 0; i < 8; ++i)
    {
      digitalWrite(m_serial_data_pin, i_bits & 0b00000001);
      digitalWrite(m_clock_pin, LOW);
      digitalWrite(m_clock_pin, HIGH);
      i_bits >>= 1;
    }
    digitalWrite(m_serial_data_pin, LOW);
  }

  const bool m_common_anode;
  const std::uint8_t m_serial_data_pin;
  const std::uint8_t m_clock_pin;
  const std::uint8_t m_clear_pin;
  static const std::array<std::uint8_t, 10> s_digits_bitmap;
};

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

SSD ssd(true, G_SHR_OUT_PIN, G_SHR_CLOCK_PIN, G_SHR_CLEAR_PIN);

void setup()
{
  Serial.begin(38400);
  ssd.Init();
  ssd.Clear();
}

void loop()
{
  for (std::uint8_t i = 0; i < 10; ++i)
  {
    ssd.put(i);
    delay(1000);
  }
}
