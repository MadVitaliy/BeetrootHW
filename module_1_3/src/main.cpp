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
    ssd.put(counter % 10);
    Serial.println("Pressed");
  }
  if (button.WasReleased())
  {
    --counter;
    ssd.put(counter % 10);
    Serial.println("Released");
  }
  if (button.IsPressedFor(1500))
  {
    const uint32_t current_timestamp = millis();
    if (current_timestamp - last_update_timestamp > 1000){
      last_update_timestamp = current_timestamp;
      ++counter;
      ssd.put(counter % 10);
      Serial.println("Pressed for");
    }
  }
}
