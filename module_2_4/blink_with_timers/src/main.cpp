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

hw_timer_t *GP_LED1_TIMER = NULL;
hw_timer_t *GP_LED2_TIMER = NULL;
hw_timer_t *GP_LED3_TIMER = NULL;

constexpr uint16_t TIMERS_PRESCALER = 40000; // Tacking clock speed of 80MHz, it gives +1 every 0.5 milli seconds.
constexpr uint16_t LED1_TIMER_TRIGGER_VALUE = 2*LED1_PIN_BLINK_HALF_PERIOD;
constexpr uint16_t LED2_TIMER_TRIGGER_VALUE = 2*LED2_PIN_BLINK_HALF_PERIOD;
constexpr uint16_t LED3_TIMER_TRIGGER_VALUE = 2*LED3_PIN_BLINK_HALF_PERIOD;



IRAM_ATTR void Led1Blinker()
{
  digitalWrite(LED1_PIN, G_LED1_STATE ? HIGH : LOW);
  G_LED1_STATE = !G_LED1_STATE;
}

IRAM_ATTR void Led2Blinker()
{
  digitalWrite(LED2_PIN, G_LED2_STATE ? HIGH : LOW);
  G_LED2_STATE = !G_LED2_STATE;
}

IRAM_ATTR void Led3Blinker()
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

  GP_LED1_TIMER = timerBegin(0,TIMERS_PRESCALER, true);
  GP_LED2_TIMER = timerBegin(1,TIMERS_PRESCALER, true);
  GP_LED3_TIMER = timerBegin(2,TIMERS_PRESCALER, true);

  timerAttachInterrupt(GP_LED1_TIMER, &Led1Blinker, true);
  timerAttachInterrupt(GP_LED2_TIMER, &Led2Blinker, true);
  timerAttachInterrupt(GP_LED3_TIMER, &Led3Blinker, true);

  timerAlarmWrite(GP_LED1_TIMER,LED1_TIMER_TRIGGER_VALUE, true );
  timerAlarmWrite(GP_LED2_TIMER,LED2_TIMER_TRIGGER_VALUE, true );
  timerAlarmWrite(GP_LED3_TIMER,LED3_TIMER_TRIGGER_VALUE, true );

  timerAlarmEnable(GP_LED1_TIMER);
  timerAlarmEnable(GP_LED2_TIMER);
  timerAlarmEnable(GP_LED3_TIMER);
}


void loop()
{
}
