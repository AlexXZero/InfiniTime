#include "components/motor/MotorController.h"
#include <hal/nrf_gpio.h>
#include "systemtask/SystemTask.h"
#include "drivers/PinMap.h"

using namespace Pinetime::Controllers;

MotorController::MotorController()
  : shortVib {1, Components::Timer::Mode::SingleShot, []{nrf_gpio_pin_set(PinMap::Motor);}},
    longVib {pdMS_TO_TICKS(1000), Components::Timer::Mode::Repeated, [this] {RunForDuration(50);}} {
}

void MotorController::Init() {
  nrf_gpio_cfg_output(PinMap::Motor);
  nrf_gpio_pin_set(PinMap::Motor);
}

void MotorController::RunForDuration(uint8_t motorDuration) {
  if (motorDuration > 0) {
    shortVib.Start(pdMS_TO_TICKS(motorDuration));
    nrf_gpio_pin_clear(PinMap::Motor);
  }
}

void MotorController::StartRinging() {
  RunForDuration(50);
  longVib.Start();
}

void MotorController::StopRinging() {
  longVib.Stop();
  nrf_gpio_pin_set(PinMap::Motor);
}
