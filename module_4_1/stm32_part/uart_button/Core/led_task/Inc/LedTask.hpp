#pragma once
#include "cmsis_os2.h"

namespace Tasks::Led {
    void Init(osMessageQueueId_t queueHandle);
    void TaskFunc(void *argument);
}
