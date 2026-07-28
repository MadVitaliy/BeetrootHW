#include "ButtonTask.hpp"
#include "Button.hpp"
#include "main.h"

namespace Tasks::Communication
{

  static osMessageQueueId_t s_target_queue = nullptr;
  static osThreadId_t s_task_handle = nullptr;
  static Button s_button_driver (0, BUTTON_GPIO_Port, BUTTON_Pin);

  void Init (osMessageQueueId_t queueHandle) {
    s_target_queue = queueHandle;

    const osThreadAttr_t attr =
      { .name = "ButtonTask", .stack_size = 128 * 4, .priority = osPriorityNormal, };
    s_task_handle = osThreadNew (TaskFunc, nullptr, &attr);
  }

  void TaskFunc (void *argument) {
    for (;;) {
      // Wait for EXTI signal
      osThreadFlagsWait (0x01, osFlagsWaitAny, osWaitForever);
      osDelay (30); // Debounce

      if (HAL_GPIO_ReadPin (BUTTON_GPIO_Port, BUTTON_Pin) == GPIO_PIN_RESET) {
        Communication::MessageType msg = Communication::MessageType::FUNCTIONAL;
        osMessageQueuePut (s_target_queue, &msg, 0, 0);
      }
    }
  }

  void OnExtiCallback (uint16_t pin) {
    if (pin == BUTTON_Pin && s_task_handle != nullptr) {
      osThreadFlagsSet (s_task_handle, 0x01);
    }
  }

}
