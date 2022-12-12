#pragma once

#include <cstdint>
#include "components/utility/Timer.h"

namespace Pinetime {
  namespace Controllers {

    class MotorController {
    public:
      MotorController();

      void Init();
      void RunForDuration(uint8_t motorDuration);
      void StartRinging();
      void StopRinging();

    private:
      Components::Timer shortVib;
      Components::Timer longVib;
    };
  }
}
