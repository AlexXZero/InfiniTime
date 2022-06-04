#include "Console.h"
#include "components/ble/BleNus.h"
#include "components/ble/NimbleController.h"
#include "systemtask/SystemTask.h"

using namespace Pinetime::Components;

Console::Console(Pinetime::System::SystemTask& systemTask,
                 Pinetime::Controllers::NimbleController& nimbleController,
                 Pinetime::Controllers::MotorController& motorController)
  : systemTask {systemTask},
    nimbleController {nimbleController},
    motorController {motorController} {
}

void Console::Init() {
  auto rxCallback = [this](const char* str, int length) {
    this->Received(str, length);
  };

  nimbleController.bleNus().RegisterRxCallback(rxCallback);
}

void Console::Print(const std::string str) {
  nimbleController.bleNus().Print(str.c_str(), str.length());
}

void Console::Process() {
}

void Console::Received(const char* str, int length) {
  enum class Shell {
    None,
    Diagnostic,
  };
  static Shell shell = Shell::None;

  for (int i = 0; i < length; i++) {
    if (shell == Shell::Diagnostic) {
      switch (str[i]) {
      case 'V': // vibrate
        motorController.RunForDuration(100);
        Print("V:\r\n");
        break;
      case 'W': // wake up
        systemTask.PushMessage(Pinetime::System::Messages::GoToRunning);
        Print("W:\r\n");
        break;
      case 'S': // sleep
        systemTask.PushMessage(Pinetime::System::Messages::GoToSleep);
        Print("S:\r\n");
        break;
      default:
        shell = Shell::None;
        Print(std::string() + str[i] + ":NI\r\n");
        break;
      }
    }

    if (shell == Shell::None) {
      switch (str[i]) {
      case 'D':
        shell = Shell::Diagnostic;
        break;
#if 0
      case '\r':
        shell = Shell::SimpleShell;
        Print("> ");
        break;
#endif
      default:
        // Do nothing
        break;
      }
    }
  }
} 
