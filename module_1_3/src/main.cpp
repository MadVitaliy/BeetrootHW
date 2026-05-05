#include <Arduino.h>
#include "configs.h"
#include "SSD.h"
#include "ButtonDebounce.h"
#include "ButtonsArray.h"

SSD G_SSD(false, G_SHR_OUT_PIN, G_SHR_CLOCK_PIN, G_SHR_CLEAR_PIN);
volatile bool G_CLEAR_DISPLAY = false;

//ButtonDebounce G_BUTTON(G_CLEAR_BUTTON_PIN, false, false);
ButtonsArray G_BUTTONS_ARRAY(G_BUTTONS_ARRAY_PIN, 4);

void IRAM_ATTR ClearDisplay()
{
  G_CLEAR_DISPLAY = true;
}

void setup()
{
  Serial.begin(38400);
  delay(100);
  G_SSD.Init();
  G_SSD.Clear();

 // G_BUTTON.Init();
  pinMode(G_CLEAR_BUTTON_PIN, INPUT);
  analogSetPinAttenuation(1, ADC_11db);
  attachInterrupt(digitalPinToInterrupt(G_CLEAR_BUTTON_PIN), ClearDisplay, FALLING);
  // Serial.println("Raw Value, Calculated Voltage, Read Voltage, Error");
}

void loop()
{
  if (G_CLEAR_DISPLAY)
  {
    G_SSD.Clear();
    G_CLEAR_DISPLAY = false;
  }

  static uint8_t previous_button = 0;
  const auto button_read = G_BUTTONS_ARRAY.ReadButton();
  if (!button_read)
    return;

  if (previous_button == button_read.value())
    return;

  previous_button = button_read.value();
  G_SSD.Put(previous_button);
}
