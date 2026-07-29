#pragma once

#include "./ModBusDataTypes.hpp"

namespace Modbus {
  /**
   * @param io_buffer_size input: size of 'o_buffer' in bytes; output: number of bytes written into 'o_buffer'
   */
  bool Serialize (const Request& i_request, uint8_t* o_buffer, size_t* io_buffer_size);
}
