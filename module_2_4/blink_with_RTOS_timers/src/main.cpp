#include <Arduino.h>

constexpr uint8_t LED1_PIN = 4;
constexpr uint8_t LED2_PIN = 5;
constexpr uint8_t LED3_PIN = 6;
constexpr uint16_t LED1_PIN_BLINK_HALF_PERIOD = 200 / 2;
constexpr uint16_t LED2_PIN_BLINK_HALF_PERIOD = 500 / 2;
constexpr uint16_t LED3_PIN_BLINK_HALF_PERIOD = 1000 / 2;

TimerHandle_t GH_LED1_TIMER;
TimerHandle_t GH_LED2_TIMER;
TimerHandle_t GH_LED3_TIMER;

IRAM_ATTR void Led1Blinker(TimerHandle_t xTimer)
{
  GPIO.out ^= 1 << LED1_PIN;
}

IRAM_ATTR void Led2Blinker(TimerHandle_t xTimer)
{
  GPIO.out ^= 1 << LED2_PIN;
}

IRAM_ATTR void Led3Blinker(TimerHandle_t xTimer)
{
  GPIO.out ^= 1 << LED3_PIN;
}

void setup()
{
  Serial.begin(115200);
  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);
  pinMode(LED3_PIN, OUTPUT);
  GPIO.out_w1tc = 1 << LED1_PIN;
  GPIO.out_w1tc = 1 << LED2_PIN;
  GPIO.out_w1tc = 1 << LED3_PIN;

  GH_LED1_TIMER = xTimerCreate(
      "BlinkLed1",                                     // Timer name
      LED1_PIN_BLINK_HALF_PERIOD / portTICK_PERIOD_MS, // 1s period
      pdTRUE,                                          // Auto-reload (periodic timer)
      NULL,                                            // Timer ID
      Led1Blinker                                      // Callback function
  );

  GH_LED2_TIMER = xTimerCreate(
      "BlinkLed3",
      LED2_PIN_BLINK_HALF_PERIOD / portTICK_PERIOD_MS,
      pdTRUE,
      NULL,
      Led2Blinker);

  GH_LED3_TIMER = xTimerCreate(
      "BlinkLed3",
      LED3_PIN_BLINK_HALF_PERIOD / portTICK_PERIOD_MS,
      pdTRUE,
      NULL,
      Led3Blinker);

  if (GH_LED1_TIMER == NULL || GH_LED2_TIMER == NULL || GH_LED3_TIMER == NULL)
  {
    Serial.println("Failed to create timer!");
    while (1)
      ;
  }

  xTimerStart(GH_LED1_TIMER, 0); // Start timer immediately
  xTimerStart(GH_LED2_TIMER, 0);
  xTimerStart(GH_LED3_TIMER, 0);
}

void loop()
{
}
