#pragma once

#include <cstdint>
#include <cstring>

extern "C"
{
#include "cmsis_os2.h"
}

namespace Tasks::Communication // TODO: try to rename to Tasks::Modbus
{
  osMessageQueueId_t Init ();
  void TaskFunc (void *argument);
  void OnExtiCallback (uint16_t pin);
}
