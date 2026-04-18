#include <Arduino.h>
#include "configs.h"
#include "SSD.h"
#include "ButtonDebounce.h"

SSD ssd(true, G_SHR_OUT_PIN, G_SHR_CLOCK_PIN, G_SHR_CLEAR_PIN);
ButtonDebounce button(G_BUTTON_PIN, true, true);

void setup()
{
  Serial.begin(38400);
  ssd.Init();
  ssd.Clear();

  button.Init();
}

void loop()
{
  static uint8_t counter = 0;
  static uint32_t last_update_timestamp = millis();
  button.Update();
  if (button.WasPressed())
  {
    ++counter;
    ssd.Put(counter%10);
    Serial.println("Pressed");
  }
  if (button.IsPressedFor(3000))
  {
    ssd.Clear();
    Serial.println("Long press");
  }
  Serial.println(counter);
}
