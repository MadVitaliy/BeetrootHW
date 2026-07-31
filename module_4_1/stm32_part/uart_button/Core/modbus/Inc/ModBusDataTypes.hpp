#pragma once

#include <cstdint>
#include <cstddef>

extern "C"
{
#include "cmsis_os.h"
}

constexpr bool G_BIG_ENDIAN = false;

namespace Modbus
{
  enum class FunctionCode : uint8_t {
    F01_READ_COILS = 1,
    F02_READ_DISCRET_INPUT = 2,
    F03_READ_HOLDING_REGISTER = 3,
    F04_READ_INPUT_REGISTER = 4,
    F05_WRITE_SINGLE_COIL = 5,
    F06_WRITE_SINGLE_HOLDING_REGISTER = 6,
    F15_WRITE_COILS = 15,
    F16_WRITE_HOLDING_REGISTERS = 16,
  };

  struct Request {
    FunctionCode fc;
    uint16_t slave_adress;
    uint16_t data_adress;
    uint8_t *data;
    size_t data_size; // depends on the fc can mean bits or bytes.
  };

  struct RequestWithHandle {
    Request request;
    osThreadId_t task_handle;
  };
}
