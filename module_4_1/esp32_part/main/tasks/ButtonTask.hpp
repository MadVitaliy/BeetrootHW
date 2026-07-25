#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Return task handle so other tasks can notify it if needed
void StartButtonTask(TaskHandle_t send_cmd_task_handle);
