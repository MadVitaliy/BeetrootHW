
#include "UartCom.hpp"
#include "cmsis_os2.h"

UartCom* UartCom::G_HANDLE = nullptr;


void ModbusTask (void *argument) {
    osMessageQueueId_t queue_handle = (osMessageQueueId_t)argument;
    s_uart_task_handle = osThreadGetId();

    UartMessage msg;

    for (;;) {
        // Wait for a transmit request
        osStatus_t status = osMessageQueueGet(queue_handle, &msg, NULL, osWaitForever);

        if (status == osOK && msg.data_size > 0) {
            // 1. Clear any stale completion flags
            osThreadFlagsClear(0x01);

            // 2. Start DMA transfer
            if (HAL_UART_Transmit_DMA(mp_huart, msg.data, msg.data_size) == HAL_OK) {

                // 3. WAIT for HAL_UART_TxCpltCallback to set flag 0x01
                osThreadFlagsWait(0x01, osFlagsWaitAny, osWaitForever);

                // 4. NOW it is safe! Notify original caller (Task 1)
                osThreadFlagsSet(msg.task_handle, 0x02);
            }
        }
    }
}

extern "C"{
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART1) { // Match your UART instance
        // Signal UartTask that hardware has finished transmitting
        osThreadFlagsSet(s_uart_task_handle, 0x01);
    }
}
}
