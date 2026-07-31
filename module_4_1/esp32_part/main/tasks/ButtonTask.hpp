#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

extern bool G_STM32_LED_STATUS; // Not what I am proud of....
// Return task handle so other tasks can notify it if needed
void StartButtonTask(TaskHandle_t send_cmd_task_handle);
