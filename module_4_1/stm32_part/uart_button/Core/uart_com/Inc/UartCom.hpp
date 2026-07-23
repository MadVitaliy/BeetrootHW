#pragma once

#include <cstdint>
#include <cstring>

extern "C"
{
#include "main.h"
#include "stm32f4xx_hal.h"
}

namespace Utils
{
  uint8_t CheckSum (const void *data, uint8_t len) {
    uint8_t temp = 0;
    for (uint8_t i = 0; i < len; i++)
      temp ^= data[i];

    return temp;
  }
}

/*
 * @brief This class implies, that connected devices are not sending data simultaneously,
 * but communicate in "message-response" way
 */
class UartCom {
  UartCom (UART_HandleTypeDef *ip_huart, DMA_HandleTypeDef *ip_hdma_usart_rx, DMA_HandleTypeDef *ip_hdma_usart_tx) :
      mp_huart (ip_huart), mp_hdma_usart_rx (ip_hdma_usart_rx), mp_hdma_usart_tx (ip_hdma_usart_tx), m_status (
          Status::IDLE) {
  }

  void Init () {
    HAL_UARTEx_ReceiveToIdle_DMA (mp_huart, mp_uart_rx_buffer, m_uart_buffer_size);
  }

  enum class Status : uint8_t {
    IDLE, TRANSMITTING, WAITING_FOR_REPONSE,
  };

  bool SendData (const void *ip_data, uint8_t i_data_size_bytes) {
    if (i_data_size_bytes > m_uart_buffer_size - 2)
      return false; // transmission of big data requires more complex handling, so omit it for now.

    uint8_t *p_buffer_head = mp_uart_tx_buffer;
    *(p_buffer_head++) = G_MESSAGE_START;
    *(p_buffer_head++) = i_data_size_bytes;

    std::memcpy (p_buffer_head, ip_data, i_data_size_bytes);
    p_buffer_head += i_data_size_bytes;

    *(p_buffer_head++) = Utils::CheckSum (ip_data, i_data_size_bytes);
    *(p_buffer_head++) = G_MESSAGE_END;

    HAL_UART_Transmit_DMA (mp_huart, mp_uart_tx_buffer, i_data_size_bytes + 2);
    m_status = Status::TRANSMITTING;
    return true;
  }

  void OnPackageSent () {
    m_status = Status::WAITING_FOR_REPONSE;
    HAL_UARTEx_ReceiveToIdle_DMA (mp_huart, mp_uart_tx_buffer, m_uart_buffer_size);
    //TODO: add timeout
  }

  void OnPackageReceived () {
    // TODO: Call task to parse or use the message
    m_status = Status::IDLE;
  }

  bool IsPackageValidMessage () {
    // Quick check first
    uint8_t *p_buffer_head = mp_uart_rx_buffer;
    if (*(p_buffer_head++) != G_MESSAGE_START)
      return false;
    const uint8_t data_lenth_bytes = *(p_buffer_head++);
    // Store an actual quantity of received bytes and check if matches;

    if (*(p_buffer_head + data_lenth_bytes) != G_MESSAGE_END)
      return false;

    // If quick checks are passed, check data integrity.
    // It would have been better to use CRC, but not this time.
    const uint8_t check_sum = Utils::CheckSum (p_buffer_head, data_lenth_bytes);
    p_buffer_head += data_lenth_bytes;
    if (*(p_buffer_head++) != check_sum)
      return false;
    return true;
  }

  static void PackageSentISR (UART_HandleTypeDef *ip_uart) {
    if (ip_uart != G_HANDLE->mp_huart)
      return;
    G_HANDLE->OnPackageSent ();
  }

  static void PackageReceivedISR (UART_HandleTypeDef *ip_uart, size_t i_data_size) {
    if (ip_uart != G_HANDLE->mp_huart)
      return;
    G_HANDLE->OnPackageReceived ();
  }

public:
private:
  constexpr size_t m_uart_buffer_size = 256;
  uint8_t mp_uart_tx_buffer[m_uart_buffer_size];
  uint8_t mp_uart_rx_buffer[m_uart_buffer_size];

  UART_HandleTypeDef *mp_huart;
  DMA_HandleTypeDef *mp_hdma_usart_rx;
  DMA_HandleTypeDef *mp_hdma_usart_tx;
  Status m_status;

  static UartCom *G_HANDLE;

  constexpr uint8_t G_MESSAGE_START = 0x02;
  constexpr uint8_t G_MESSAGE_END = 0x03;
};

// TODO: add instructions
namespace Communication
{
  /**
   * @brief A logical protol is 2-byte message, sent by MainControlUnit.
   * The first byte has
   */
  enum class MessageType : uint8_t {
    NONE, // in case no messages has been received;
    MENU,
    ENTER,
    ESCAPE,
    ARROWS,    // + byte of arrows;
    FUNCTIONAL // + a single byte number of functional key;
  };
  /**
   * @brief OsdUnit answers to every message from MainControlUnit (except ARROWS).
   * The MCU usually keeps sending a message till a moment it receives an OK response.
   * Usually, but not always. F.E. it is OK if some ARROWS messages have been lost.
   */
  enum class Response : uint8_t {
    OK, CORRUPTED
  };
}



