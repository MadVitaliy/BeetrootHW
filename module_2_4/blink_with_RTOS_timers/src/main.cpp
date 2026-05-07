#include <Arduino.h>

constexpr uint8_t LED1_PIN = 4;
constexpr uint8_t LED2_PIN = 5;
constexpr uint8_t LED3_PIN = 6;
constexpr uint16_t LED1_PIN_BLINK_HALF_PERIOD = 200 / 2;
constexpr uint16_t LED2_PIN_BLINK_HALF_PERIOD = 500 / 2;
constexpr uint16_t LED3_PIN_BLINK_HALF_PERIOD = 1000 / 2;
volatile bool G_LED1_STATE = false;
volatile bool G_LED2_STATE = false;
volatile bool G_LED3_STATE = false;

TimerHandle_t GH_LED1_TIMER;
TimerHandle_t GH_LED2_TIMER;
TimerHandle_t GH_LED3_TIMER;

constexpr uint16_t TIMERS_PRESCALER = 40000; // Tacking clock speed of 80MHz, it gives +1 every 0.5 milli seconds.
constexpr uint16_t LED1_TIMER_TRIGGER_VALUE = 2 * LED1_PIN_BLINK_HALF_PERIOD;
constexpr uint16_t LED2_TIMER_TRIGGER_VALUE = 2 * LED2_PIN_BLINK_HALF_PERIOD;
constexpr uint16_t LED3_TIMER_TRIGGER_VALUE = 2 * LED3_PIN_BLINK_HALF_PERIOD;

IRAM_ATTR void Led1Blinker(TimerHandle_t xTimer)
{
  digitalWrite(LED1_PIN, G_LED1_STATE ? HIGH : LOW);
  G_LED1_STATE = !G_LED1_STATE;
}

IRAM_ATTR void Led2Blinker(TimerHandle_t xTimer)
{
  digitalWrite(LED2_PIN, G_LED2_STATE ? HIGH : LOW);
  G_LED2_STATE = !G_LED2_STATE;
}

IRAM_ATTR void Led3Blinker(TimerHandle_t xTimer)
{
  digitalWrite(LED3_PIN, G_LED3_STATE ? HIGH : LOW);
  G_LED3_STATE = !G_LED3_STATE;
}

void setup()
{
  Serial.begin(115200);
  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);
  pinMode(LED3_PIN, OUTPUT);

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
  xTimerStart(GH_LED2_TIMER, 0); // Start timer immediately
  xTimerStart(GH_LED3_TIMER, 0); // Start timer immediately
}

void loop()
{
}
