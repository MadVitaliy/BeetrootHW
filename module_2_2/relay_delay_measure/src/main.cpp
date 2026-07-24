#include <Arduino.h>
#include "soc/gpio_struct.h"
#include "driver/gpio.h"
#include <cstdint>

constexpr uint8_t G_RELEAY_CONTROL_PIN = 5;
constexpr uint8_t G_RELEAY_OUT_READER_PIN = 4;

volatile bool G_RELAY_WAS_CLOSED = false;
void RelayIsTrigggered()
{
  G_RELAY_WAS_CLOSED = true;
}

void setup()
{
  Serial.begin(38400);
  delay(1000);

  pinMode(G_RELEAY_CONTROL_PIN, OUTPUT);

  pinMode(G_RELEAY_OUT_READER_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(G_RELEAY_OUT_READER_PIN), RelayIsTrigggered, FALLING);
}

enum class STATE
{
  ST_TEST,
  ST_SLEEP,
};

uint32_t trigger_time, closing_time, calculated_delay;
STATE state = STATE::ST_SLEEP;

void Test()
{
 
  uint32_t current_timestamp = trigger_time;
  bool relay_was_closed = false;
  uint8_t trigger_counter = 0;

  do 
  {
    current_timestamp = millis();
    if (G_RELAY_WAS_CLOSED)
    {
      relay_was_closed = true;
      closing_time = current_timestamp;
      ++trigger_counter;
      G_RELAY_WAS_CLOSED = false;
    }
  } while (current_timestamp - trigger_time < 2000);

  calculated_delay = closing_time - trigger_time;
  if (relay_was_closed)
  {
    Serial.print("Relay close delay is ");
    Serial.println(calculated_delay);
    Serial.print("Relay was triggered time: ");
    Serial.println(trigger_counter);
  }
  else
    Serial.println("Relay wa not closed");
}

void loop()
{
  if (state == STATE::ST_SLEEP)
  {
    Serial.println("Sleep");
    GPIO.out_w1tc = (1 << G_RELEAY_CONTROL_PIN);
    delay(2000);
    state = STATE::ST_TEST;
    G_RELAY_WAS_CLOSED = false;
    Serial.println("Test");
    trigger_time = millis();
    GPIO.out_w1ts = (1 << G_RELEAY_CONTROL_PIN);
  }

  if (state == STATE::ST_TEST)
  {
    Test();
    state = STATE::ST_SLEEP;
  }
}
