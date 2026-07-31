#include "ButtonTask.hpp"
//#include "Button.hpp"
#include "ModBusDataTypes.hpp"

extern "C"
{
#include "main.h"
}

typedef StaticTask_t osStaticThreadDef_t;

namespace Tasks::Led
{
  static osMessageQueueId_t s_target_queue = nullptr;
  //static Button s_button_driver (0, BUTTON_GPIO_Port, BUTTON_Pin);

  osThreadId_t led_task_handle;

  uint32_t led_task_buffer[128];
  osStaticThreadDef_t led_task_control_block;
  const osThreadAttr_t led_task_attributes =
    { .name = "led_task",                                 //
        .cb_mem = &led_task_control_block,                  //
        .cb_size = sizeof(led_task_control_block),          //
        .stack_mem = &led_task_buffer[0],                  //
        .stack_size = sizeof(led_task_buffer),             //
        .priority = (osPriority_t) osPriorityBelowNormal1,  //
      };

  void LedTask (void *argument) {
    uint16_t data_buffer[10];

    Modbus::RequestWithHandle request;
    request.request.fc = Modbus::FunctionCode::F03_READ_HOLDING_REGISTER;
    request.request.slave_adress = 1;
    request.request.data_adress = 1;
    request.request.data_size = 1; // It's not data_buffer size, but a number of registers, to be read;
    request.request.data = reinterpret_cast<uint8_t*> (data_buffer);
    request.task_handle = osThreadGetId ();
    for (;;) {
      osDelay (1000); // Read ESP32's button status every 500ms
      osMessageQueuePut (s_target_queue, &request, 0, 0);

      uint32_t result = osThreadFlagsWait (0x05, osFlagsWaitAny, 3000);
      if (result == osFlagsErrorTimeout)
        continue;
      else if ((result & osFlagsError) != 0)
        continue;

      bool led_status = data_buffer[6];
      if (led_status)
        HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
      else
        HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
    }
  }

  void Init (osMessageQueueId_t queueHandle) {
    s_target_queue = queueHandle;
    led_task_handle = osThreadNew (LedTask, nullptr, &led_task_attributes);
  }

}
