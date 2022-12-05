#include "Console.h"
#include "components/ble/BleNus.h"
#include "components/ble/NimbleController.h"
#include "components/eventlog/EventLog.h"
#include "systemtask/SystemTask.h"

using namespace Pinetime::Components;

Console::Console(Pinetime::System::SystemTask& systemTask,
                 Pinetime::Controllers::NimbleController& nimbleController,
                 Pinetime::Controllers::MotorController& motorController,
                 Pinetime::Components::EventLog& eventlog)
  : systemTask {systemTask},
    nimbleController {nimbleController},
    motorController {motorController},
    eventlog {eventlog} {
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

static char process_cmd = 0;
static EventLogIterator event;

void Console::Process() {
  static std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds> prev_time;
  auto cur_time = eventlog.dateTimeController.CurrentDateTime();
  if (std::chrono::duration_cast<std::chrono::milliseconds>(cur_time - prev_time).count() < 20) {
    // avoid spamming
    systemTask.PushMessage(Pinetime::System::Messages::ConsoleProcess);
    return;
  }
  prev_time = cur_time;

  switch (process_cmd) {
  case 'E':
    if (event == eventlog.end()) {
      Print("E:\r\n");
    } else {
      char buf[32];
      snprintf(buf, sizeof(buf), "E: %u,%08x\r\n", event.Index(), event.Value());
      Print(buf);
      ++event;
      systemTask.PushMessage(Pinetime::System::Messages::ConsoleProcess);
      return;
    }
    break;
  default:
    Print(std::string() + process_cmd + ":NI\r\n");
    break;
  }
  process_cmd = 0;
}

void Console::Received(const char* str, int length) {
  enum class Shell {
    None,
    Diagnostic,
  };
  static Shell shell = Shell::None;

  for (int i = 0; i < length; i++) {
    if (shell == Shell::Diagnostic) {
      process_cmd = 0;
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
      case 'E':
        process_cmd = str[i];
        event = eventlog.begin();
        systemTask.PushMessage(Pinetime::System::Messages::ConsoleProcess);
        break;
      case 'R':
        eventlog.Write<Event::Simple>(SimpleEvent::SoftwareReset);
        Print("R:\r\n");
        break;
      case 'T':
        eventlog.SwapPages();
        Print("T:\r\n");
        break;
      case 'P':
        eventlog.EraseAll();
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
