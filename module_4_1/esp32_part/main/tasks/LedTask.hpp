#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Return task handle so other tasks can notify it if needed
TaskHandle_t StartLedTask(void);
