#include <Arduino.h>

#include "driver/ledc.h"

constexpr uint8_t G_POTENTIOMETER_PIN = 5;

constexpr uint8_t G_MOTOR_CONTROL_PIN = 4;
constexpr ledc_mode_t G_PWM_MODE = LEDC_LOW_SPEED_MODE;
constexpr ledc_channel_t G_PWM_CHANNEL = LEDC_CHANNEL_0;
constexpr uint8_t G_PWM_RESOLUTION = 10; // bits (1–20 on ESP32)
constexpr uint32_t G_PWM_MAX_VALUE = (1 << G_PWM_RESOLUTION) - 1;
constexpr uint32_t G_PWM_MIN_VALUE = G_PWM_MAX_VALUE * 0.3; // Imperically found minimum to rotate the motor
constexpr uint32_t G_PWM_FREQ = 20000;
constexpr int G_HPOINT = 0;

template <size_t N>
class MovingAverage
{
private:
  size_t index = 0;
  size_t count = 0;
  uint32_t sum = 0;
  uint16_t buffer[N] = {0};

public:
  uint16_t add(uint16_t value)
  {
    // Subtract the value being overwritten
    sum -= buffer[index];
    // Insert the new value
    buffer[index] = value;
    sum += value;
    // Move index forward (circularly)
    index = (index + 1) % N;
    return getAverage();
  }

  uint16_t getAverage() const
  {
    return sum / N;
  }
};

void pwm_init(void)
{
  ledc_timer_config_t timer = {
      .speed_mode = G_PWM_MODE,             // or LOW_SPEED
      .duty_resolution = LEDC_TIMER_10_BIT, // 1–20 bits
      .timer_num = LEDC_TIMER_0,
      .freq_hz = G_PWM_FREQ, // PWM frequency
      .clk_cfg = LEDC_AUTO_CLK};
  ledc_timer_config(&timer);
}

void pwm_channel_init(void)
{
  ledc_channel_config_t channel = {
      .gpio_num = G_MOTOR_CONTROL_PIN,
      .speed_mode = G_PWM_MODE,
      .channel = G_PWM_CHANNEL,
      .intr_type = LEDC_INTR_DISABLE,
      .timer_sel = LEDC_TIMER_0,
      .duty = 0, // initial duty
      .hpoint = G_HPOINT};
  ledc_channel_config(&channel);
}

void pwm_set_duty(uint32_t duty)
{
  ledc_set_duty(G_PWM_MODE, G_PWM_CHANNEL, duty);
  ledc_update_duty(G_PWM_MODE, G_PWM_CHANNEL);
}

MovingAverage<8> filter;

void setup()
{
  pinMode(G_MOTOR_CONTROL_PIN, OUTPUT);
  pwm_init();
  pwm_channel_init();
  ledc_fade_func_install(0);
  Serial.begin(38400);

  ledc_timer_config_t timer_config{};
  ledc_timer_config({});
  Serial.print("G_MOTOR_CONTROL_PIN=");
  Serial.println(G_MOTOR_CONTROL_PIN);
  Serial.print("G_PWM_CHANNEL=");
  Serial.println(G_PWM_CHANNEL);
  Serial.print("G_PWM_RESOLUTION=");
  Serial.println(G_PWM_RESOLUTION);
  Serial.print("G_PWM_MAX_VALUE=");
  Serial.println(G_PWM_MAX_VALUE);
  Serial.print("G_PWM_FREQ=");
  Serial.println(G_PWM_FREQ);
  delay(5000);
}

void loop()
{
  static uint16_t prev_duty = 0;
  const uint16_t raw_value = analogRead(G_POTENTIOMETER_PIN);
  const uint16_t new_duty = filter.add(map(raw_value, 0, 4095, G_PWM_MIN_VALUE, G_PWM_MAX_VALUE));
  if (new_duty != prev_duty)
  {
    prev_duty = new_duty;
    pwm_set_duty(new_duty);
    Serial.println(new_duty);
  }
}
