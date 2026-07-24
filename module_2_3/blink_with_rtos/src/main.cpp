#include <Arduino.h>

constexpr uint8_t LED1_PIN = 4;
constexpr uint8_t LED2_PIN = 5;
constexpr uint8_t LED3_PIN = 6;
constexpr uint16_t LED1_PIN_BLINK_HALF_PERIOD = 200 / 2;
constexpr uint16_t LED2_PIN_BLINK_HALF_PERIOD = 500 / 2;
constexpr uint16_t LED3_PIN_BLINK_HALF_PERIOD = 1000 / 2;

void BlinkLed1(void *parameter)
{
  for (;;)
  {
    // GPIO.out_w1ts = 1 << LED1_PIN;
    Serial.println("Turning on LED1");
    digitalWrite(LED1_PIN, HIGH);
    vTaskDelay(LED1_PIN_BLINK_HALF_PERIOD / portTICK_PERIOD_MS);
    // GPIO.out_w1tc = 1 << LED1_PIN;
    Serial.println("Turning off LED1");
    digitalWrite(LED1_PIN, LOW);
    vTaskDelay(LED1_PIN_BLINK_HALF_PERIOD / portTICK_PERIOD_MS);
  }
}

void BlinkLed2(void *parameter)
{
  for (;;)
  {
    // GPIO.out_w1ts = 1 << LED1_PIN;
    Serial.println("Turning on LED2");
    digitalWrite(LED2_PIN, HIGH);
    vTaskDelay(LED2_PIN_BLINK_HALF_PERIOD / portTICK_PERIOD_MS);
    // GPIO.out_w1tc = 1 << LED1_PIN;
    Serial.println("Turning off LED2");
    digitalWrite(LED2_PIN, LOW);
    vTaskDelay(LED2_PIN_BLINK_HALF_PERIOD / portTICK_PERIOD_MS);
  }
}

void BlinkLed3(void *parameter)
{
  for (;;)
  {
    // GPIO.out_w1ts = 1 << LED1_PIN;
    Serial.println("Turning on LED3");
    digitalWrite(LED3_PIN, HIGH);
    vTaskDelay(LED3_PIN_BLINK_HALF_PERIOD / portTICK_PERIOD_MS);
    // GPIO.out_w1tc = 1 << LED1_PIN;
    Serial.println("Turning off LED3");
    digitalWrite(LED3_PIN, LOW);
    vTaskDelay(LED3_PIN_BLINK_HALF_PERIOD / portTICK_PERIOD_MS);
  }
}

void setup()
{
  Serial.begin(115200);
  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);
  pinMode(LED3_PIN, OUTPUT);
  xTaskCreate(
      BlinkLed1,   // Function that should be called
      "BlinkLed1", // Name of the task (for debugging)
      1000,        // Stack size words
      NULL,        // Parameter to pass
      1,           // Task priority
      NULL         // Task handle
  );
  xTaskCreate(
      BlinkLed2,
      "BlinkLed2",
      1000,
      NULL,
      1,
      NULL);
  xTaskCreate(
      BlinkLed3,
      "BlinkLed3",
      1000,
      NULL,
      1,
      NULL);
}

void loop()
{
  // put your main code here, to run repeatedly;
  // Hold my beer: it's empty;
}
