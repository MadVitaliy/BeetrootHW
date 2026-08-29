#include <Arduino.h>

constexpr uint8_t LED1_PIN = 4;
constexpr uint8_t LED2_PIN = 5;
constexpr uint8_t LED3_PIN = 6;
constexpr uint16_t LED1_PIN_BLINK_HALF_PERIOD = 200 / 2;
constexpr uint16_t LED2_PIN_BLINK_HALF_PERIOD = 500 / 2;
constexpr uint16_t LED3_PIN_BLINK_HALF_PERIOD = 1000 / 2;

hw_timer_t *GP_LED1_TIMER = NULL;
hw_timer_t *GP_LED2_TIMER = NULL;
hw_timer_t *GP_LED3_TIMER = NULL;

constexpr uint16_t TIMERS_PRESCALER = 40000; // Tacking clock speed of 80MHz, it gives +1 every 0.5 milli seconds.
constexpr uint16_t LED1_TIMER_TRIGGER_VALUE = 2 * LED1_PIN_BLINK_HALF_PERIOD;
constexpr uint16_t LED2_TIMER_TRIGGER_VALUE = 2 * LED2_PIN_BLINK_HALF_PERIOD;
constexpr uint16_t LED3_TIMER_TRIGGER_VALUE = 2 * LED3_PIN_BLINK_HALF_PERIOD;

IRAM_ATTR void Led1Blinker()
{
  GPIO.out ^= 1 << LED1_PIN;
}

IRAM_ATTR void Led2Blinker()
{
  GPIO.out ^= 1 << LED2_PIN;
}

IRAM_ATTR void Led3Blinker()
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

  GP_LED1_TIMER = timerBegin(0, TIMERS_PRESCALER, true);
  GP_LED2_TIMER = timerBegin(1, TIMERS_PRESCALER, true);
  GP_LED3_TIMER = timerBegin(2, TIMERS_PRESCALER, true);

  timerAttachInterrupt(GP_LED1_TIMER, &Led1Blinker, true);
  timerAttachInterrupt(GP_LED2_TIMER, &Led2Blinker, true);
  timerAttachInterrupt(GP_LED3_TIMER, &Led3Blinker, true);

  timerAlarmWrite(GP_LED1_TIMER, LED1_TIMER_TRIGGER_VALUE, true);
  timerAlarmWrite(GP_LED2_TIMER, LED2_TIMER_TRIGGER_VALUE, true);
  timerAlarmWrite(GP_LED3_TIMER, LED3_TIMER_TRIGGER_VALUE, true);

  timerAlarmEnable(GP_LED1_TIMER);
  timerAlarmEnable(GP_LED2_TIMER);
  timerAlarmEnable(GP_LED3_TIMER);
}

void loop()
{
}
