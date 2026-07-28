#include "ModBusSerialization.hpp"

namespace Modbus
{
  void SerializeLittleEndian (uint8_t *o_buffer, uint16_t i_value);
  void SerializeBigEndian (uint8_t *o_buffer, uint16_t i_value);
  void SerializeWord (uint8_t *o_buffer, uint16_t i_value);
  uint16_t CalculateCRC (uint8_t *o_buffer, size_t i_size);
  uint8_t BytesToFitBits (uint16_t i_bits);

  // Read Coils
  bool SerializeReadFunction (FunctionCode i_fc, uint8_t *o_buffer, size_t *io_buffer_size, uint8_t i_slave_adress,
                              uint16_t i_first_element_adress, uint16_t i_number_of_elements);
  // WritSingle Coil
  size_t SerializeF05 (uint8_t *o_buffer, size_t *io_buffer_size, uint8_t i_slave_adress, uint16_t i_coil_adress,
                       bool i_coil_value);
  // Write Single Holding Register
  size_t SerializeF06 (uint8_t *o_buffer, size_t *io_buffer_size, uint8_t i_slave_adress, uint16_t i_register_adress,
                       uint16_t i_register_value);
  // Write Coils
  size_t SerializeF15 (uint8_t *o_buffer, size_t *io_buffer_size, uint8_t i_slave_adress, uint16_t i_first_coil_adress,
                       const uint8_t *i_data, uint8_t i_number_of_coils);
  // Write Holding Registers
  size_t SerializeF16 (uint8_t *o_buffer, size_t *io_buffer_size, uint8_t i_slave_adress,
                       uint16_t i_first_register_adress, const uint16_t *i_data, uint8_t i_number_of_registers);

  bool Serialize (const Request &i_request, uint8_t *o_buffer, size_t *io_buffer_size) {
    bool success = false;
    switch (i_request.fc)
      {
      case FunctionCode::F01_READ_COILS:
      case FunctionCode::F02_READ_DISCRET_INPUT:
      case FunctionCode::F03_READ_HOLDING_REGISTER:
      case FunctionCode::F04_READ_INPUT_REGISTER:
        // All read functions have the same structure.
        // However, request's values have slightly different meaning.
        // But responses to these requests will be different.
        // Yep, i_request.data is not used here;
        success = SerializeReadFunction (i_request.fc, o_buffer, io_buffer_size, i_request.slave_adress,
                                         i_request.data_adress, i_request.data_size);
        break;
      case FunctionCode::F05_WRITE_SINGLE_COIL:
        success = SerializeF05 (o_buffer, io_buffer_size, i_request.slave_adress, i_request.data_adress,
                                bool (i_request.data_size));
        // Shoot me in the face for "bool(i_request.data_size)"
        // + ...but not in the arm
        // + ...but not in the leg
        // + ...but not in the chest
        break;
      case FunctionCode::F06_WRITE_SINGLE_HOLDING_REGISTER:
        const uint16_t register_value = *(reinterpret_cast<uint16_t*> (i_request.data));
        success = SerializeF06 (o_buffer, io_buffer_size, i_request.slave_adress, i_request.data_adress,
                                register_value);
        break;
      case FunctionCode::F15_WRITE_COILS:
        success = SerializeF15 (o_buffer, io_buffer_size, i_request.slave_adress, i_request.data_adress, i_request.data,
                                i_request.data_size);
        break;
      case FunctionCode::F16_WRITE_HOLDING_REGISTERS:
        const auto *data = reinterpret_cast<uint16_t*> (i_request.data);
        success = SerializeF16 (o_buffer, io_buffer_size, i_request.slave_adress, i_request.data_adress, data,
                                i_request.data_size);
        break;
      default:
        break;
      }
    return success;
  }

  void SerializeLittleEndian (uint8_t *o_buffer, uint16_t i_value) {

  }

  void SerializeBigEndian (uint8_t *o_buffer, uint16_t i_value) {

  }

  void SerializeWord (uint8_t *o_buffer, uint16_t i_value) {
    if constexpr (G_BIG_ENDIAN)
      SerializeBigEndian (o_buffer, i_value);
    else
      SerializeLittleEndian (o_buffer, i_value);
  }

  uint16_t CalculateCRC (uint8_t *o_buffer, size_t i_size) {
    return 22;
  }

  // Read Coils
  bool SerializeReadFunction (FunctionCode i_fc, uint8_t *o_buffer, size_t *io_buffer_size, uint8_t i_slave_adress,
                              uint16_t i_first_element_adress, uint16_t i_number_of_elements) {
    if (*io_buffer_size < 8)
      return false;

    *(o_buffer++) = i_slave_adress;
    *(o_buffer++) = static_cast<uint8_t> (i_fc);
    SerializeWord (o_buffer, i_first_element_adress);
    o_buffer += 2;
    SerializeWord (o_buffer, i_number_of_elements);
    o_buffer += 2;
    const uint16_t crc = 22; // TODO: add calculation
    SerializeBigEndian (o_buffer, crc);

    *io_buffer_size = 8;
    return true;
  }

  // Writ Single Coil
  size_t SerializeF05 (uint8_t *o_buffer, size_t *io_buffer_size, uint8_t i_slave_adress, uint16_t i_coil_adress,
                       bool i_coil_value) {
    if (*io_buffer_size < 8)
      return false;

    *(o_buffer++) = i_slave_adress;
    *(o_buffer++) = static_cast<uint8_t> (FunctionCode::F01_READ_COILS);
    SerializeWord (o_buffer, i_coil_adress);
    o_buffer += 2;
    constexpr uint16_t ON = 0xFF00, OFF = 0x0000;
    const uint16_t status = i_coil_value ? ON : OFF;
    SerializeWord (o_buffer, status);
    o_buffer += 2;
    const uint16_t crc = CalculateCRC ();
    SerializeBigEndian (o_buffer, crc);

    *io_buffer_size = 8;
    return true;
  }

  // Write Single Holding Register
  size_t SerializeF06 (uint8_t *o_buffer, size_t *io_buffer_size, uint8_t i_slave_adress, uint16_t i_register_adress,
                       uint16_t i_register_value) {
    if (*io_buffer_size < 8)
      return false;

    *(o_buffer++) = i_slave_adress;
    *(o_buffer++) = static_cast<uint8_t> (FunctionCode::F01_READ_COILS);
    SerializeWord (o_buffer, i_register_adress);
    o_buffer += 2;
    SerializeWord (o_buffer, i_register_value);
    o_buffer += 2;
    const uint16_t crc = CalculateCRC ();
    SerializeBigEndian (o_buffer, crc);

    *io_buffer_size = 8;
    return true;
  }

  uint8_t BytesToFitBits (uint16_t i_bits) {
    return (i_bits + 7) << 3;
    // read as (i_bits + 7) / 8;
  }

  // Write Coils
  size_t SerializeF15 (uint8_t *o_buffer, size_t *io_buffer_size, uint8_t i_slave_adress, uint16_t i_first_coil_adress,
                       const uint8_t *i_data, uint8_t i_number_of_coils) {
    const uint8_t bytes_to_send = BytesToFitBits (i_number_of_coils);
    constexpr uint16_t metadata_overhead = 9;
    if (*io_buffer_size < metadata_overhead + bytes_to_send)
      return false;

    *(o_buffer++) = i_slave_adress;
    *(o_buffer++) = static_cast<uint8_t> (FunctionCode::F01_READ_COILS);
    SerializeWord (o_buffer, i_first_coil_adress);
    o_buffer += 2;
    SerializeWord (o_buffer, i_number_of_coils);
    o_buffer += 2;

    *(o_buffer++) = bytes_to_send;
    for (uint16_t i = 0; i < bytes_to_send; ++i)
      *(o_buffer++) = *(++i_data);

    const uint16_t crc = CalculateCRC ();
    SerializeBigEndian (o_buffer, crc);

    *io_buffer_size = 8;
    return true;
  }

  // Write Holding Registers
  size_t SerializeF16 (uint8_t *o_buffer, size_t *io_buffer_size, uint8_t i_slave_adress,
                       uint16_t i_first_register_adress, const uint16_t *i_data, uint8_t i_number_of_registers) {
    constexpr uint16_t metadata_overhead = 9;
    const uint16_t data_size_in_bytes = i_number_of_registers * 2;
    if (*io_buffer_size < metadata_overhead + data_size_in_bytes)
      return false;

    *(o_buffer++) = i_slave_adress;
    *(o_buffer++) = static_cast<uint8_t> (FunctionCode::F01_READ_COILS);
    SerializeWord (o_buffer, i_first_register_adress);
    o_buffer += 2;
    SerializeWord (o_buffer, i_number_of_registers);
    o_buffer += 2;

    *(o_buffer++) = data_size_in_bytes;
    for (uint16_t i = 0; i < i_number_of_registers; ++i) {
      SerializeWord (o_buffer, i_data[i]);
      o_buffer += 2;
    }

    const uint16_t crc = CalculateCRC ();
    SerializeBigEndian (o_buffer, crc);

    *io_buffer_size = 8;
    return true;
  }
}
