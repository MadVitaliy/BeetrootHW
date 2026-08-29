#include <Arduino.h>
#include "esp_sleep.h"

constexpr gpio_num_t RELAY_PIN = GPIO_NUM_4;
constexpr uint8_t WORKING_TIME = 15;             // in seconds
constexpr uint8_t DOWN_TIME = 60 - WORKING_TIME; // in seconds

constexpr uint32_t SECONDS_2_MICROSECONDS{1000000ull};
constexpr uint32_t WORKING_TIME_MCS = WORKING_TIME * SECONDS_2_MICROSECONDS;
constexpr uint32_t DOWN_TIME_MCS = DOWN_TIME * SECONDS_2_MICROSECONDS;

RTC_DATA_ATTR bool G_DOWNTIME = true;

hw_timer_t *GP_RELAY_TIMER = NULL;

IRAM_ATTR void HandleRaley()
{
  // GPIO.out ^= 1 << LED1_PIN;
}

void printWakeupReason()
{
  esp_sleep_wakeup_cause_t reason;

  reason = esp_sleep_get_wakeup_cause();

  switch (reason)
  {
  case ESP_SLEEP_WAKEUP_TIMER:
    Serial.println("Wakeup caused by timer");
    break;

  default:
    Serial.println("Normal startup");
    break;
  }
}

void setup()
{
  G_DOWNTIME = !G_DOWNTIME;

  Serial.begin(115200);
  delay(100);
  
  printWakeupReason();
  
  if (G_DOWNTIME)
  Serial.println("Fan turned off");
  else
  Serial.println("Fan turned on");
  
  
  Serial.flush();
  
  gpio_hold_dis(RELAY_PIN);
  
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, G_DOWNTIME ? LOW : HIGH);
  
  gpio_hold_en(RELAY_PIN);
  gpio_deep_sleep_hold_en();


  const auto sleep_time = G_DOWNTIME ? DOWN_TIME_MCS : WORKING_TIME_MCS;
  esp_sleep_enable_timer_wakeup(sleep_time);
  esp_deep_sleep_start();
}

void loop()
{
}
