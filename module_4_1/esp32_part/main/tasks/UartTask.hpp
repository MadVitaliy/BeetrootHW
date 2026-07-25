#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

struct UartTaskHandles {
    TaskHandle_t send_task;
    TaskHandle_t read_task;
};

// We pass led_task_handle so UART read task knows who to notify
UartTaskHandles StartUartTasks(TaskHandle_t led_task_handle);
