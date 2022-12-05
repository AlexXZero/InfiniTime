#pragma once

#include "components/utility/Timer.h"

namespace Pinetime {
  namespace System {
    class SystemTask;
  }
  namespace Controllers {

    class TimerController {
    public:
      TimerController();

      void Init(System::SystemTask* systemTask);

      void StartTimer(uint32_t duration);

      void StopTimer();

      uint32_t GetTimeRemaining();

      bool IsRunning();

      void OnTimerEnd();

    private:
      System::SystemTask* systemTask = nullptr;
      Components::Timer timer;
    };
  }
}
