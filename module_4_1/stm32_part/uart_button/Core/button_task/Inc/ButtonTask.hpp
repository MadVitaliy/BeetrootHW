#pragma once
#include "cmsis_os2.h"

namespace Tasks::Button {
    void Init(osMessageQueueId_t queueHandle);
    void TaskFunc(void *argument);
    void OnExtiCallback(uint16_t pin);
}
