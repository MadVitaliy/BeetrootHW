#include "ButtonTask.hpp"
//#include "Button.hpp"
#include "ModBusDataTypes.hpp"

extern "C"
{
#include "main.h"
}

static osMessageQueueId_t s_target_queue = nullptr;
static osThreadId_t s_button_task_handle = nullptr;
typedef StaticTask_t osStaticThreadDef_t;
namespace Tasks::Button
{
  //static Button s_button_driver (0, BUTTON_GPIO_Port, BUTTON_Pin);

  osThreadId_t led_task_handle;

  uint32_t led_task_buffer[128];
  osStaticThreadDef_t led_task_control_block;
  const osThreadAttr_t led_task_attributes =
    { .name = "ButtonTask",                                 //
        .cb_mem = &led_task_control_block,                  //
        .cb_size = sizeof(led_task_control_block),          //
        .stack_mem = &led_task_buffer[0],                  //
        .stack_size = sizeof(led_task_buffer),             //
        .priority = (osPriority_t) osPriorityBelowNormal1,  //
      };

  void ButtonTask (void *argument) {
    uint16_t data = 3;

    Modbus::RequestWithHandle request;
    request.request.fc = Modbus::FunctionCode::F06_WRITE_SINGLE_HOLDING_REGISTER;
    request.request.slave_adress = 1;
    request.request.data_adress = 1;
    request.request.data_size = 1;
    request.request.data = reinterpret_cast<uint8_t*>(&data);
    request.task_handle = osThreadGetId ();
    for (;;) {
      // Wait for EXTI signal
      osThreadFlagsWait (0x01, osFlagsWaitAny, osWaitForever);
      osDelay (30);   /// Debounce
      if (HAL_GPIO_ReadPin (BUTTON_GPIO_Port, BUTTON_Pin) == GPIO_PIN_RESET) {
        osMessageQueuePut (s_target_queue, &request, 0, 0);
      }
    }
  }

  void Init (osMessageQueueId_t queueHandle) {
    s_target_queue = queueHandle;
    s_button_task_handle = osThreadNew (ButtonTask, nullptr, &led_task_attributes);
  }

}

extern "C"
{
  void OnExtiCallback (uint16_t pin) {
    if (pin == BUTTON_Pin && s_button_task_handle != nullptr)
      osThreadFlagsSet (s_button_task_handle, 0x01);
  }

  void HAL_GPIO_EXTI_Callback (uint16_t GPIO_Pin) {
    if (GPIO_Pin == BUTTON_Pin && s_button_task_handle != nullptr) {
      osThreadFlagsSet (s_button_task_handle, 0x01);
    }
  }
}

