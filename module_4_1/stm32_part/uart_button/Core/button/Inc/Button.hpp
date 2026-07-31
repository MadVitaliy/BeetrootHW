#pragma once

#include <cstdint>
#include <cstring>

extern "C"
{
#include "main.h"
#include "stm32f4xx_hal.h"
}

class Button {
public:
  Button (size_t i_id, GPIO_TypeDef *ip_gpio_port, uint16_t i_pin) :
      m_id (i_id), mp_gpio_port (ip_gpio_port), m_pin (i_pin) {
    GP_REGISTRY[m_id] = this;
  }

  bool WasPressed () {
    return m_was_pressed;
  }

  void Update () {
    // TODO: check pin, update state of the button
  }

  static void ButtonISR (size_t i_id, uint16_t i_gpio_pin) {
    if (i_id >= G_MAX_BUTTONS_QUANTITY || GP_REGISTRY[i_id]->m_pin != i_gpio_pin)
      return;
    GP_REGISTRY[i_id]->Update ();
  }

private:
  size_t m_id;
  GPIO_TypeDef *mp_gpio_port;
  uint16_t m_pin;
  bool m_was_pressed;

  static constexpr size_t G_MAX_BUTTONS_QUANTITY = 5;
  static Button *GP_REGISTRY[G_MAX_BUTTONS_QUANTITY];
};
