#pragma once

#include <cstdint>
#include <cstddef>
#include <string>

namespace Pinetime {

  namespace System {
    class SystemTask;
  }

  namespace Controllers {
    class NimbleController;
    class MotorController;
  }

  namespace Components {
    class EventLog;

    class Console {
    public:
      Console(Pinetime::System::SystemTask& systemTask,
              Pinetime::Controllers::NimbleController& nimbleController,
              Pinetime::Controllers::MotorController& motorController,
              Pinetime::Components::EventLog& eventlog);

      void Init();
      void Process();
      void Print(std::string str);
      void Received(const char* str, int length);

    private:
      Pinetime::System::SystemTask& systemTask;
      Pinetime::Controllers::NimbleController& nimbleController;
      Pinetime::Controllers::MotorController& motorController;
      Pinetime::Components::EventLog& eventlog;
    };
  }
}
