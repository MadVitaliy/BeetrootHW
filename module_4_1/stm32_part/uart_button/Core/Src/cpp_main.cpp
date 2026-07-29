#include <cpp_main.hpp>
#include <cstdint>

extern "C"
{
#include "cmsis_os.h"
}

#include "ButtonTask.hpp"
#include "ModBusTasks.hpp"


void AppInit (void) {
  osMessageQueueId_t modbus_requst_queue = Tasks::Communication::Init();
  Tasks::Button::Init(modbus_requst_queue);
}
