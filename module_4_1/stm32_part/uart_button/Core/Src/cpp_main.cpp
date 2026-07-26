#include "cpp_main.h"

#include <cstdint>

extern "C"
{
#include "main.h"
#include "cmsis_os.h"
#include "stm32f4xx_hal.h"
}

#include "UartCom.hpp"
#include "Button.hpp"

extern UART_HandleTypeDef huart2;
extern DMA_HandleTypeDef hdma_usart2_rx;
extern DMA_HandleTypeDef hdma_usart2_tx;

extern osThreadId_t defaultTaskHandle;
extern osThreadId_t ButtonTaskHandle;
extern osThreadId_t UartComunicatioHandle;

Button G_BUTTON (0, BUTTON_GPIO_Port, BUTTON_Pin);
UartCom G_UART_COM (&huart2, &hdma_usart2_rx, &hdma_usart2_tx);

osMessageQueueId_t G_MESSAGE_QUEUE_HANDLE;

extern "C"
{
  void StartDefaultTask (void *argument) {
    for (;;) {
      osDelay (1);
    }
  }

  void StartButtonTask (void *argument) {
    for (;;) {
      osThreadFlagsWait (0x01, osFlagsWaitAny, osWaitForever);
      osDelay (30);   // Debounce delay
      if (HAL_GPIO_ReadPin (BUTTON_GPIO_Port, BUTTON_Pin) == GPIO_PIN_RESET) {
        // Confirm button is still pressed
        Communication::MessageType message = Communication::MessageType::FUNCTIONAL;
        osMessageQueuePut (G_MESSAGE_QUEUE_HANDLE, &message, 0, 0);
      }
    }
  }

  void StartUartComunication (void *argument) {
    for (;;) {
      Communication::MessageType message = Communication::MessageType::FUNCTIONAL;
      osStatus_t status = osMessageQueueGet (G_MESSAGE_QUEUE_HANDLE, &message, NULL, osWaitForever);
      if (status == osOK) {

        G_UART_COM.SendData( (&message), sizeof(Communication::MessageType));

      }
    }
  }

  void HAL_GPIO_EXTI_Callback (uint16_t GPIO_Pin) {
    if (GPIO_Pin == BUTTON_Pin) {
      osThreadFlagsSet (ButtonTaskHandle, 0x01);
    }
  }

  void HAL_UARTEx_RxEventCallback (UART_HandleTypeDef *ip_huart, uint16_t i_size) {
    UartCom::PackageReceivedISR (ip_huart, i_size);
  }

  void HAL_UART_TxCpltCallback (UART_HandleTypeDef *ip_huart) {
    UartCom::PackageSentISR (ip_huart);
  }
}

