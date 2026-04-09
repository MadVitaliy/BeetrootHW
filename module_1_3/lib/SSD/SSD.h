#pragma once

#include <array>
#include <cstdint>

class SSD
{
  // Has nothing in common with solid-state drive.
  // Stands for 7-segments display.
public:
  SSD(bool i_common_anode, std::uint8_t i_serial_data_pin,
      std::uint8_t i_clock_pin,
      std::uint8_t i_clear_pin) ;
  void Init();
  void Clear();
  void Put(std::uint8_t i_digit);
  
private:
  void SendBits(std::uint8_t i_bits);
  const bool m_common_anode;
  const std::uint8_t m_serial_data_pin;
  const std::uint8_t m_clock_pin;
  const std::uint8_t m_clear_pin;
  const std::uint32_t m_serial_data_pin_bitmask;
  const std::uint32_t m_clock_pin_bitmask;
  const std::uint32_t m_clear_pin_bitmask;
  static const std::array<std::uint8_t, 10> s_digits_bitmap;
};

