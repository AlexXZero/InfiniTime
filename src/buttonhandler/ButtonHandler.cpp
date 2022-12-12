#include "buttonhandler/ButtonHandler.h"

using namespace Pinetime::Controllers;

void ButtonTimerCallback(TimerHandle_t xTimer) {
  auto* sysTask = static_cast<Pinetime::System::SystemTask*>(pvTimerGetTimerID(xTimer));
  sysTask->PushMessage(Pinetime::System::Messages::HandleButtonTimerEvent);
}

void ButtonHandler::Init(Pinetime::System::SystemTask* systemTask) {
  buttonTimer = Components::Timer {pdMS_TO_TICKS(200), [systemTask] {
    systemTask->PushMessage(Pinetime::System::Messages::HandleButtonTimerEvent);
  }};
}

ButtonActions ButtonHandler::HandleEvent(Events event) {
  static constexpr TickType_t doubleClickTime = pdMS_TO_TICKS(200);
  static constexpr TickType_t longPressTime = pdMS_TO_TICKS(400);
  static constexpr TickType_t longerPressTime = pdMS_TO_TICKS(2000);

  if (event == Events::Press) {
    buttonPressed = true;
  } else if (event == Events::Release) {
    releaseTime = xTaskGetTickCount();
    buttonPressed = false;
  }

  switch (state) {
    case States::Idle:
      if (event == Events::Press) {
        buttonTimer.Start(doubleClickTime);
        state = States::Pressed;
      }
      break;
    case States::Pressed:
      if (event == Events::Press) {
        if (xTaskGetTickCount() - releaseTime < doubleClickTime) {
          buttonTimer.Stop();
          state = States::Idle;
          return ButtonActions::DoubleClick;
        }
      } else if (event == Events::Release) {
        buttonTimer.Start(doubleClickTime);
      } else if (event == Events::Timer) {
        if (buttonPressed) {
          buttonTimer.Start(longPressTime - doubleClickTime);
          state = States::Holding;
        } else {
          state = States::Idle;
          return ButtonActions::Click;
        }
      }
      break;
    case States::Holding:
      if (event == Events::Release) {
        buttonTimer.Stop();
        state = States::Idle;
        return ButtonActions::Click;
      } else if (event == Events::Timer) {
        buttonTimer.Start(longerPressTime - longPressTime - doubleClickTime);
        state = States::LongHeld;
        return ButtonActions::LongPress;
      }
      break;
    case States::LongHeld:
      if (event == Events::Release) {
        buttonTimer.Stop();
        state = States::Idle;
      } else if (event == Events::Timer) {
        state = States::Idle;
        return ButtonActions::LongerPress;
      }
      break;
  }
  return ButtonActions::None;
}
